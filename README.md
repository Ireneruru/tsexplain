# TSExplain: Explaining Aggregated Time Series by Surfacing Evolving Contributors

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
> TSExplain helps you find the evolving explanations of the aggregated time series! 

Aggregated time series are generated effortlessly everywhere, e.g., "total confirmed covid-19 cases since 2019" and "total liquor sales over time." Understanding "how" and "why" these key performance indicators (KPI) evolve over time is critical to making data-informed decisions. Existing explanation engines focus on explaining one aggregated value or the difference between two relations. However, this falls short of explaining KPIs' continuous changes over time. ***TSEXPLAIN is a system that explains aggregated time series by surfacing the underlying evolving top contributors.***

You can find related [full paper](https://arxiv.org/pdf/2211.10909.pdf) published at ICDE 2023 and the [demo paper](https://dl.acm.org/doi/abs/10.1145/3514221.3520153) published at SIGMOD. 

[Project page](https://www.cs.columbia.edu/~chen1ru/TSExplain.html)

## Demo
https://user-images.githubusercontent.com/13096451/235597495-aa3ab80d-4d41-4a3b-9142-c786aa9bb278.mp4


## Compile TSExplain

```
mkdir build
cd build
cmake ..
make
```

## Install Python dependencies

```
pip install matplotlib streamlit
```

## Run the demo interface

```
cd demo 

streamlit run demo.py
```


## Citation 
```
@article{chen2022tsexplain,
  title={TSEXPLAIN: Explaining Aggregated Time Series by Surfacing Evolving Contributors},
  author={Chen, Yiru and Huang, Silu},
  journal={arXiv preprint arXiv:2211.10909},
  year={2022}
}

@inproceedings{chen2021tsexplain,
  title={Tsexplain: Surfacing evolving explanations for time series},
  author={Chen, Yiru and Huang, Silu},
  booktitle={Proceedings of the 2021 International Conference on Management of Data},
  pages={2686--2690},
  year={2021}
}
```

