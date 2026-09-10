# CRT Sparse Sort (Thanh Ha Algorithm)

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.22657400.svg)](https://doi.org/10.5281/zenodo.22657400)

A domain-specific $O(N)$ non-comparative sorting framework based on the Chinese Remainder Theorem (CRT) and 2D Coordinate Space Mapping $(S, L)$. Designed for high-performance microarchitectures, pipeline hardware accelerators (FPGA/ASIC), and cyclic signal processing systems (DSP/Radar).

---

## 📁 Repository Structure

| File Name | Description |
| :--- | :--- |
| **`ThanhHaAlgorithm.pdf`** | Full scientific paper manuscript in English (IEEEtran format). |
| **`ThanhHaAlgorithm_Vietnamese.pdf`** | Full scientific paper manuscript in Vietnamese (IEEEtran format). |
| **`Q&A.pdf`** | Comprehensive Q&A document explaining the mathematical foundation, edge cases, and algorithmic design. |
| **`main.cpp`** | Sequential C++11 implementation of the CRT Sparse Sort ($O(N)$ non-comparative algorithm). |
| **`main_parallel.cpp`** | Multi-threaded parallel C++ implementation utilizing OpenMP for CPU multi-core acceleration. |
| **`main_gen_input.cpp`** | Utility script to generate test datasets (`crt.inp`) with 64-bit random integers. |

---

## 🔄 What's New in v1.1.1 (vs v1.1.0)

* **Lock-Free Thread-Local Buffering**: Replaced critical sections and global locks with thread-local vector buffering (`local_lower` / `local_upper`) and prefix-sum array assembly. Achieves zero thread contention during 2D coordinate calculation.
* **Parallel 2-Pointer Merge via Binary Search**: Introduced `find_split_A` algorithm to split the lower group ($A$) and upper mirror group ($B$) into independent contiguous chunks, enabling lock-free parallel merging across CPU cores.
* **Optimized 64-bit Key Packing Schema (`28|31|5`)**: Restructured bit allocation to fit perfectly into a single `uint64_t` primitive type, eliminating memory padding and maximizing L1/L2 cache line throughput.
* **Strict Theoretical Domain Recalibration**: Standardized upper bound $V_{\max}$ to exactly $2^{28} \times M_{\text{TOTAL}} - 1$, reflecting hardware-aligned 28-bit Quotient indexing.

---

## 🚀 Key Theoretical Features

* **2D Coordinate Mapping $(S, L)$**: Decomposes 1D integers into segment index $S$ and local phase $L$, preserving 100% monotonic algebraic order without requiring pairwise comparison operators.
* **Mirror Symmetry Split**: Reflects residue values across the CRT period midpoint ($M_{\text{MID}}$), saving 50% coordinate storage space while keeping $O(N)$ unmirroring overhead.
* **64-bit Monotonic Packed Key**: Packs Quotient Key $Q_{\text{key}}$ (28-bit), Segment $S$ (31-bit), and Local Offset $L$ (5-bit) into a single `uint64_t` variable to preserve monotonic order during parallel radix passes.
* **Branch-Free Execution**: Eliminates control-flow branches inside the core loop, ensuring deterministic execution time and optimization for L1 Cache locality.

---

## 📊 Supported Data Range & Limits

The strict 64-bit packed key encoding ($28 | 31 | 5$) guarantees accurate monotonic sorting for numbers up to:

$$V_{\max} = 2^{28} \times M_{\text{TOTAL}} - 1 = 868,367,236,112,416,767 \approx 868.37 \times 10^{15}$$

* **Range Coverage**: Supports unsigned integers up to ~868 Quadrillion (~868 Trẻn).
* **Packed Key Layout (`uint64_t`)**:
  * `Bit [36..63]` (28 bits): Inverted/Direct Quotient Key $Q_{\text{key}}$ ($0 \le Q \le 2^{28}-1$)
  * `Bit [5..35]` (31 bits): Segment Index $S$ ($1 \le S \le 1,699,060,562$)
  * `Bit [0..4]` (5 bits): Local Offset $L$ ($0 \le L \le 28$)

---

## 🛠️ Quick Start & Usage

### 1. Generate Input Data
Compile and run `main_gen_input.cpp` to create the test dataset file (`crt.inp`):

```bash
g++ -O3 main_gen_input.cpp -o gen_input
./gen_input
```

### 2. Execute Sequential Version
Compile and run `main.cpp` for single-threaded sorting:

```bash
g++ -O3 main.cpp -o crt_sort
./crt_sort
```

### 3. Execute Parallel Version (OpenMP)
Compile with -fopenmp and run `main_parallel.cpp` for multi-core CPU acceleration:

```bash
g++ -O3 -fopenmp main_parallel.cpp -o crt_sort_parallel
./crt_sort_parallel
```
---

## 📖 Citation

If you use this algorithm or reference this work in your research, please cite:

```bibtex
@article{nguyen2026crtspsort,
  title={CRT Sparse Sort (Thanh Ha Algorithm): A Non-Comparative
         Sorting Framework via Chinese Remainder Theorem and 2D
         Coordinate Mapping for Hardware Accelerators},
  author={Nguyen, Hao T.},
  year={2026},
  publisher={GitHub},
  howpublished={\url{[https://github.com/nthaobn/CRT-Sparse-Sort](https://github.com/nthaobn/CRT-Sparse-Sort)}}
}
```

---

**Author:** Hao T. Nguyen (Nguyễn Thanh Hào)  
*Dedicated to my beloved daughter, Nguyen Ngoc Thanh Ha.*
