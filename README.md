# Cache-Behavior-Simulation
This is a C++ program that simulates the cache behavior on MIPS instruction set architecture and evaluates its performance using sample data. The project focuses on cache indexing policy, which is a key aspect of CPU performance.

## Topic
Traditional cache indexing policies take the least significant bits as indexing bits due to locality, which works well for general purpose applications. However, for embedded systems designed for specific use cases, choosing indexing bits based on frequently-used operations may yield better results. This program implements an indexing policy based on the paper "Zero Cost Indexing for Improved Processor Cache Performance" by Tony Givargis (2006). The Least Recently Used (LRU) policy is assumed, meaning that the block staying the longest in a full cache set will be replaced when another block is brought into the cache set.

## Purpose
The purpose of this project is to simulate and analyze the cache behavior on MIPS instruction set architecture with the zero-cost indexing policy, and evaluate its performance using test data. By implementing and testing this indexing policy, we can gain insights into how it compares to traditional indexing policies and understand its potential impact on the performance of embedded systems.
