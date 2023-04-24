#include <iostream>
#include <math.h>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

// #define DEBUG // 取消註解會輸出過程

using namespace std;

struct Block
{
    int tag;
    bool valid;        // has data?
    int lastUsedCount; // record when the block be accessed
};

class Cache
{
private:
    int addressBits;                 // # of bits
    int blockSize;                   // (Byte)
    int cacheSets;                   // # of set (= # of blocks / set size)
    int associativity;               // # of blocks per set (= set size)
    int tagBits;                     // # of tag bit
    int comBits;                     // paper use
    int setBitCount;                 // # of set index bit
    int offsetBitCount;              // # of block offset bit
    vector<vector<Block>> sets;      // store data
    vector<string> references;       // store the addr we want
    vector<vector<int>> bitsTable;   // # of row of table -> # of reference addr
    vector<float> QualityTable;      // quality measurement
    vector<vector<float>> CorrTable; // 0 = Same, 1 = Complete not same
    vector<int> indexingPattern;

public:
    /**
     * @param addressBits # of bits
     * @param blockSize (Byte)
     * @param cacheSets s# of set
     * @param associativity # of blocks per set = set size
     */
    Cache(int addressBits, int blockSize, int cacheSets, int associativity) : addressBits{addressBits}, blockSize{blockSize}, cacheSets{cacheSets}, associativity{associativity}
    {
        offsetBitCount = (int)log2(blockSize);
        setBitCount = (int)log2(cacheSets);
        tagBits = addressBits - offsetBitCount;
        comBits = tagBits + offsetBitCount;

#ifdef DEBUG
        cout << "setBitCount: " << setBitCount << "\n";
        cout << "offsetBitCount: " << offsetBitCount << "\n";
        cout << "comBits: " << comBits << "\n";
        cout << "tagBits: " << tagBits << "\n\n";
#endif // DEBUG
        InitCache();
    }

    /**
     * initialize Cache
     *
     * associativity = # of blocks / per set
     * total # of block = cacheSets x block 
     */
    void InitCache()
    {
        for (int i = 0; i < cacheSets; i++)
        {
            sets.emplace_back(); // add empty vector i.e set placement 
            for (int j = 0; j < associativity; j++)
            {
                sets[i].emplace_back(Block{-1, false, 0}); // 預設 block
            }
        }
    }

    /**
     * Page. 11, generate Striped Trace File & Quality Measures
     *
     */
    void CalculateQuality()
    {
        float r;
        float Zeros; // # of 0
        float Ones;  // # of 1  = bitsTable.size() - Zeros

        for (int i = 0; i < comBits; i++)
        {
            Zeros = Ones = 0; 
            for (size_t j = 0; j < bitsTable.size(); j++)
            {
                (bitsTable[j][i] == 1) ? Ones++ : Zeros++;
            }

            r = Zeros < Ones ? Zeros / Ones : Ones / Zeros;
            QualityTable.push_back(r);
        }
    }

    /**
     * Page. 11-12 -> calculate the correlation between abitrary 2 addr bits 
     *
     */
    void CalculateCorrelation()
    {
        for (int i = 0; i < comBits; i++)
        {
            vector<float> correlation; 

            for (int j = 0; j < i; j++)
            {
                correlation.push_back(CorrTable[j][i]);
            }
            correlation.push_back(0);

            for (int j = i + 1; j < comBits; j++)
            {
                float r;
                float Eij = 0; // # of same
                float Dij = 0; // # of diff = bitsTable.size() - Eij
                for (size_t k = 0; k < bitsTable.size(); k++)
                {
                    (bitsTable[k][i] == bitsTable[k][j]) ? Eij++ : Dij++;
                }

                r = Eij < Dij ? Eij / Dij : Dij / Eij;
                correlation.push_back(r);
            }
            CorrTable.push_back(correlation);
        }
    }

    /**
     * Algorithm :
     * Near-optimal Index Ordering
     */
    void NOIO()
    {
        vector<bool> used(addressBits, false); // avoid the addr bit be used repeatly
        indexingPattern.resize(offsetBitCount);

        // find offsetBitCount 個 index
        for (int i = 0; i < offsetBitCount; i++)
        {
            float maxQuality = -1;
            int maxQIndex = -1;

            // find the best Q & record its index ->反過來由MSB開始看 
            for (int j = comBits - 1; j >= 0; j--)
            {
                if (QualityTable[j] > maxQuality && !used[j])
                {
                    maxQuality = QualityTable[j];
                    maxQIndex = j;
                }
            }

            // refresh the bit position
            indexingPattern[i] = addressBits - maxQIndex; // 因反過來(由MSB)先看的, 額外處理
            used[maxQIndex] = true;
            cout << "maxQIndex: " << maxQIndex << "\n";

            // refresh the quality
            for (int j = 0; j < tagBits; j++)
            {
                QualityTable[j] *= CorrTable[j][maxQIndex];
            }
        }

        // 排序後比較好存取
        sort(indexingPattern.begin(), indexingPattern.end());

#ifdef DEBUG
        for (auto &r : references)
        {
            cout << r << " ";
            for (auto &i : indexingPattern)
            {
                cout << "a" << i << "(" << r[i] << ") ";
            }
            cout << endl;
        }
#endif // DEBUG
    }

    /**
     * if cache has data ->True, otherwiase ->False and replace
     *
     * @param address 
     *
     * @return bool, Hit or not?
     */
    bool Access(string address)
    {
        bool hit = false;       // hit ?
        int lastUsedCount = -1; // find the last used cache 次序 (0, 1, 2...)
        int lruBlockIndex = -1; // find the last used cache index

        // handel bit = 0 condition
        int tag = tagBits == 0 ? 0 : stoi(address.substr(0, tagBits), 0, 2);                            // tag
        int indexingBitCount = setBitCount == 0 ? 0 : stoi(address.substr(tagBits, setBitCount), 0, 2); // = cache index

#ifdef DEBUG
        cout << " tag: " << tag << " indexingBitCount: " << indexingBitCount << "\n";
#endif // DEBUG
        int insertToBlock = 0;
        for (size_t i = 0; i < indexingPattern.size(); i++)
        {
            insertToBlock += (address[indexingPattern[i]] - '0') << (indexingPattern.size() - 1 - i);
        }

#ifdef DEBUG
        cout << "Allocation #" << insertToBlock << " Block" << endl;
#endif // DEBUG

        /* save all blocks of current set */
        for (int i = 0; i < associativity; i++)
        {
            /* check valid ? & Hit ? */
            if (sets[insertToBlock][i].valid && sets[insertToBlock][i].tag == tag)
            {
                hit = true;                               // store data
                sets[insertToBlock][i].lastUsedCount = 0; // 歸零
                continue;
            }

            /* find the last used addr (data) */
            if (sets[insertToBlock][i].lastUsedCount > lastUsedCount)
            {
                lastUsedCount = sets[insertToBlock][i].lastUsedCount;
                lruBlockIndex = i;
            }

            sets[insertToBlock][i].lastUsedCount++; // 使用次序增加
        }

        if (!hit)
        {
            /* 如果沒有有效的記憶體就直接取用第一個就好 */
            if (lruBlockIndex == -1)
            {
                lruBlockIndex = lastUsedCount = 0;
            }

            /* 如果使用記憶體的次序是否超過有效範圍就歸零 */
            if (lastUsedCount >= setBitCount)
            {
                lastUsedCount = 0;
            }

            sets[insertToBlock][lruBlockIndex] = {tag, true, lastUsedCount++}; // 設定記憶體值
        }
        printTable();

#ifdef DEBUG
        cout << (hit ? "hit" : "miss") << "\n";
#endif // DEBUG
        return hit;
    }

    int Simlate(ofstream &o_file)
    {
        int totalMissCount = 0;
        for (auto &r : references)
        {
            if (Access(r))
            {
                o_file << r << " hit\n"; 
            }
            else
            {
                totalMissCount++;
                o_file << r << " miss\n"; 
            }
        }

        return totalMissCount;
    }

    /* print out temporary table */
    void printTable()
    {
        for (int i = 0; i < cacheSets; i++)
        {
            for (int j = 0; j < associativity; j++)
            {
                cout << " |" << sets[i][j].tag;
            }
            cout << " |" << endl
                 << "-----------" << endl;
        }
    }

    /**
     * 紀錄存取的記憶體並計算01數量
     *
     * @param r
     */
    void AddReference(string r)
    {
        references.push_back(r);

        vector<int> bits;

        // 只需要紀錄offsetbit以前的
        for (int i = addressBits - 1; i >= 0; i--)
        {
            bits.push_back(r[i] - '0');
        }
        bitsTable.push_back(bits);
    }

    /*  def Getter */
    int &getIndexBitCount()
    {
        return setBitCount;
    }

    int &getOffsetBitCount()
    {
        return offsetBitCount;
    }

    int &getTagBits()
    {
        return tagBits;
    }

    int &getAddressBits()
    {
        return addressBits;
    }
};

int main(int argc, char *argv[])
{
    int Address_bits, blockSize, cacheSets, associativity;
    int totalMissCount = 0;  
    Cache *cache;            
    string tmp;              // for reading file
    ifstream c_file, r_file; // read file
    ofstream o_file;         // output file

#ifdef DEBUG
    c_file.open("cache.org", ios::in);
    r_file.open("reference.lst", ios::in);
    o_file.open("test.txt", ios::out);
#else
    c_file.open(argv[1], ios::in);
    r_file.open(argv[2], ios::in);
    o_file.open(argv[3], ios::out);
#endif // DEBUG

    c_file >> tmp >> Address_bits;
    c_file >> tmp >> blockSize;
    c_file >> tmp >> cacheSets;
    c_file >> tmp >> associativity;

    cache = new Cache(Address_bits, blockSize, cacheSets, associativity);

    // output the commpleted data
    o_file << "Address bits: " << Address_bits << "\n";
    o_file << "Block size: " << blockSize << "\n";
    o_file << "Cache sets: " << cacheSets << "\n";
    o_file << "Associativity: " << associativity << "\n\n";

    o_file << "Offset bit count: " << cache->getOffsetBitCount() << "\n";
    o_file << "Indexing bit count: " << cache->getIndexBitCount() << "\n";
    o_file << "Indexing bits:";
    for (int i = 0; i < cache->getIndexBitCount(); ++i)
    {
        o_file << " " << (cache->getAddressBits() - 1 - cache->getIndexBitCount() - i);
    }
    o_file << "\n\n";

    // read .benchmark 行
    getline(r_file, tmp);
    o_file << tmp << "\n";

    // read reference data
    while (r_file >> tmp)
    {
        // if沒讀取到結尾符號'.'就繼續
        if (tmp[0] == '.')
        {
            break;
        }
        cache->AddReference(tmp);
    }

    // calculate Zero Cost Index Ordering

    cache->CalculateQuality();
    cache->CalculateCorrelation();
    cache->NOIO();
    totalMissCount = cache->Simlate(o_file);

    o_file << ".end\n\nTotal cache miss count: " << totalMissCount;

    // close file
    c_file.close();
    r_file.close();
    o_file.close();
    return 0;
}