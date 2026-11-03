# AttrVecSim
AttrVecSim is a discrete-event simulator of attribute-vector routing protocols, developed as part of the MSc dissertation *SyRuP: A Multipath Routing Protocol for the Path-Aware Network Architecture*, at Instituto Superior Técnico, Universidade de Lisboa (2026)

# Directory Structure
This project follows the [Pitchfork](https://joholl.github.io/pitchfork-website/) layout for C++ projects, with merged header placement, as most of the simulator is templated, and therefore largely header-only. The project, with emphasis on its folder partitioning, is structured as follows.

```bash
AttrVecSim/
├── data/                   
│   ├── datasets/           # simulator ready synthetic and inferred network datasets
│   ├── plots/              # generated figures
│   ├── output/             # results produced by the simulator
│   └── topologies/         # Rocketfuel & small scale SCIERA topologies
├── external/
│   └── tabulate.hpp        # third-party library
├── src/        
│   ├── core/               # program entry point
│   ├── graph/              # network model
│   ├── protocol/           # Proto. implementation: SyRuP, SyRuP-LFN, SyRuP-W, BGP, BGP-RCN, beaconing
│   ├── routing-algebras/   # routing algebra logic
│   ├── scheduler/          # event scheduler driving the simulation
│   ├── simulator/          # sets up and executes a single simulation run
│   └── utils/              # helper functions, namely, UI and csv data recording
└── tools/                  
    ├── scripts/            # scripts to produce datasets and run simulations in batch
    └── plotting/           # scripts to plot the results
```
