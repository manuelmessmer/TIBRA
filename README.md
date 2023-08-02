# QuESo - Quadrature for Embedded Solids

[![License: MIT](https://img.shields.io/badge/License-BSD4-green.svg)](https://github.com/manuelmessmer/QuESo/blob/main/LICENSE) [![C++][c++-image]][c++standard] [![Build Status](https://github.com/manuelmessmer/QuESo/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/manuelmessmer/QuESo/actions)

[c++-image]: https://img.shields.io/badge/C++-17-blue.svg?style=flat&logo=c%2B%2B
[c++standard]: https://isocpp.org/std/the-standard

QuESo (formerly TIBRA) is a preprocessor to generate analysis-ready embedded finite element models from arbitrarily complex solids described in boundary representation (B-Rep). The solid geometry is processed as a standard STL file. QuESo is designed to provide highly efficient quadrature rules that can be used in any embedded or immersed boundary method. To this end, the STL model is embedded into the background mesh defined by a regular grid, i.e., with hexahedral integration domains (finite elements). The user can specify the polynomial degree and the number of elements in each spatial direction. QuESo reads the STL file and computes the integration points required for an FE analysis. The integration points have the following characteristics:
* All integration weights are positive.
* Point locations are restricted to the material domain.
* The number of points per cut element is always $n \leq (p+1)^3$.
  
Both classical $C^0$ finite elements and isogeometric elements defined on trivariate B-Spline domains are supported. An interface to the FE framework [Kratos Multiphysics](https://github.com/KratosMultiphysics/Kratos) is provided. QuESo is written in C++ and has a user-friendly Python interface. More information about the theoretical background of QuESo can be found in [1]. Please do not hesitate to contact me with questions about QuESo.

If you want to learn how to use QuESo, check out the [Wiki](https://github.com/manuelmessmer/QuESo/wiki/Getting-Started). Additionally, there are several examples in [Examples](https://github.com/manuelmessmer/QuESo/tree/main/examples).


Input: Solid B-Rep Model (STL)             |  Output: Quadrature Rules
:-------------------------:|:-------------------------:
![](https://github.com/manuelmessmer/QuESo/blob/main/docs/brep.png)  |  ![](https://github.com/manuelmessmer/QuESo/blob/main/docs/iga_model.png)

## How to cite QuESo?
Please use the following references when citing QuESo in your work.
- [1] Manuel Meßmer, Tobias Teschemacher, Lukas F. Leidinger, Roland Wüchner, Kai-Uwe Bletzinger, Efficient CAD-integrated isogeometric analysis of trimmed solids, Comput. Methods Appl. Mech. Engrg. 400 (2022) 115584, https://doi.org/10.1016/j.cma.2022.115584.
- [2] Manuel Meßmer, Lukas F. Leidiner, Stefan Hartmann, ..., Kai-Uwe Bletzinger, Isogeometric Analysis on Trimmed Solids: A B-Spline-Based Approach Focusing on Explicit Dynamics, 13th European LS-DYNA Conference, Ulm, Germany, 2021. [Meßmer et al. 2022](https://www.researchgate.net/publication/357053531_Isogeometric_Analysis_on_Trimmed_Solids_A_B-Spline-Based_Approach_Focusing_on_Explicit_Dynamics).
- [3] Manuel Meßmer, QuESo, https://github.com/manuelmessmer/QuESo.



