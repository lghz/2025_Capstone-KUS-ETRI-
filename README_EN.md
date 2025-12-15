# 2025 Capstone – Research & Development on Cryptographic Applications of Machine Learning

> Tree Parity Machine (TPM)-based Neural Key Exchange PoC

This project was implemented by referencing the papers listed in the **References** section below.
This repository is the outcome of the capstone topic **“Research & Development on Cryptographic Applications of Machine Learning”**, where we implement a **Tree Parity Machine (TPM)**-based **Neural Key Exchange** proof-of-concept and summarize/compare synchronization characteristics across different learning rules.
Unlike conventional key exchange schemes based on mathematical hard problems (e.g., DH), this work focuses on implementing the process in which **two nodes synchronize to the same key (weights) through learning**.

---

## Team

* **Faculty Advisor**: Taegeun Kim (Korea University)
* **PL**: Sangsu Lee (ETRI)
* **Members**: Gunhyung Lim (Team Lead), Chaehan Yang, Jubin Kim, Taeheun Kim
* **Affiliation**: Department of Artificial Intelligence & Cybersecurity, Korea University

---

## Development Environment

* OS: Ubuntu 22.04.5 LTS
* Language: Python 3.12.x, C

---

## Architecture

Two nodes communicate over TCP, and each node consists of the following modules:

* **TCP Module**: TCP communication / session management between nodes
* **TPM Module (ML)**: TPM-based key generation (synchronization) and weight update logic

---

## TPM-based Key Exchange Workflow (Summary)

1. Each node **initializes secret/random weights**.
2. Nodes generate/exchange **public input vectors**.
3. Each node computes TPM outputs using the input and its weights, and **updates weights only when the update condition is satisfied**.
4. By repeating output exchange/comparison, the process continues until **weights are synchronized**, which is treated as successful key agreement.
5. Communication may terminate under certain conditions (e.g., repeated update-condition failures).

### Key Hyperparameters (Example)

> Example setting in the referenced paper: **K=3, N=4, L=3**

* **K**: number of hidden units
* **N**: number of inputs per hidden unit
* **L**: weight range (e.g., L=3)
* The final key is represented as a **weight vector (Weight Vector)**.

---

## Implemented Learning Rules

This repository implements and compares representative learning rules used for TPM synchronization:

* **Random Walk**: a conservative update rule that shifts weights by small steps when the relevant values match
* **Query**: uses adjusted/controlled inputs (rather than fully random inputs) to improve synchronization behavior
* **Anti-Hebbian**: updates in a way that mitigates weight bias toward a single direction

---

## Experiments / Results (Summary)

We compare synchronization characteristics and efficiency across learning rules using:

* **Sync Iterations**: number of iterations required to synchronize
* **Repulsive Steps**: occurrences/patterns of updates or states that hinder synchronization

---

## Usage (Quick Note)

> Detailed execution commands/arguments/I/O format will be added after finalizing the repository structure.

* Run from each implementation directory (Random Walk / Query / Anti-Hebbian)
* Launch two nodes and connect them via TCP to verify synchronization results

---

## References

* Bocheng Bao, Haigang Tang, Yuanhui Su, Han Bao, Mo Chen, Quan Xu, “Two-Dimensional Discrete Bi-Neuron Hopfield Neural Network With Polyhedral Hyperchaos,” IEEE, 2024, 5907–5918.
* Di Xiao, Xiaofeng Liao, Yong Wang, “Parallel keyed hash function construction based on chaotic neural network,” Neurocomputing, 2009, 2288–2296.
* Andreas Ruttor, Wolfgang Kinzel, Ido Kanter, “Neural cryptography with queries,” Journal of Statistical Mechanics: Theory and Experiment, 2005, P01009.
* Xuguang Wu, Yiliang Han, Minqing Zhang, ShuaiShuai Zhu, Su Cui, Yuanyuan Wang, Yixuan Peng, “Pseudorandom number generators based on neural networks: a review,” (Apr 2025).
