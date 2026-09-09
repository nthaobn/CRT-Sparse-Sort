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

## 🚀 Key Theoretical Features

* **2D Coordinate Mapping $(S, L)$**: Decomposes 1D integers into segment index $S$ and local phase $L$, preserving 100% monotonic algebraic order without requiring pairwise comparison operators.
* **Mirror Symmetry Split**: Reflects residue values across the CRT period midpoint ($M_{\text{MID}}$), saving 50% coordinate storage space while keeping $O(N)$ unmirroring overhead.
* **64-bit Monotonic Packed Key**: Packs Quotient $Q$ (32-bit), Mirror Bit $M$ (1-bit), Segment $S$ (26-bit), and Local Offset $L$ (5-bit) into a single `uint64_t` variable to preserve monotonic order during parallel sorting passes.
* **Branch-Free Execution**: Eliminates control-flow branches inside the core loop, ensuring deterministic execution time and optimization for L1 Cache locality.

---

## 📊 Supported Data Range & Limits

The 64-bit packed key encoding supports strict monotonic sorting for values up to:

$$V_{\max} = 2^{32} \times M_{\text{TOTAL}} - 1 = 13,893,025,849,152,012,287 \approx 13.89 \times 10^{18}$$

* **Range Coverage**: ~75.3% of the complete 64-bit unsigned integer space (`uint64_t`).
* **Packed Key Layout**:
  * `Bit [32..63]` (32 bits): Quotient $Q$ ($0 \le Q \le 2^{32}-1$)
  * `Bit [31]` (1 bit): Mirror Flag $M$ ($0$ or $1$)
  * `Bit [5..30]` (26 bits): Inverted/Direct Segment Index $S_{\text{key}}$ ($0 \le S \le 55,773,217$)
  * `Bit [0..4]` (5 bits): Inverted/Direct Local Offset $L_{\text{key}}$ ($0 \le L \le 28$)

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
