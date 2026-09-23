# RPBOB

```bash
find . -name "*.csv" -type f -delete
./run_simulations.sh -d bclique -u -N 5 --regen
./run_simulations.sh -d rocketfuel --as "1221" --regen -N 2
./run_simulations.sh -d rocketfuel   # uses default AS list
```

```
├── core
│   ├── argparsing.cpp
│   ├── argparsing.hpp
│   ├── config.hpp
│   └── main.cpp
├── graph
├── protocol
│   ├── beaconing
│   ├── bgp
│   ├── bgp-rcn
│   ├── rpbob
│   └── protocol.hpp
├── routing-algebras
│   ├── extensions
│   ├── orders
│   ├── lengths.hpp
│   └── seq.hpp
├── scheduler
│   ├── calendar.hpp
│   ├── event.hpp
│   └── rng.hpp
├── simulator
│   ├── instances
│   │   ├── becing-simulator.hpp
│   │   ├── bgp-simulator.hpp
│   │   ├── rcn-simulator.hpp
│   │   └── rpbob-simulator.hpp
│   ├── simulator-common.hpp
│   └── simulator.hpp
└── utils
    ├── helpers
    ├── stats
    └── ui

```

./build/Simulator -f data/datasets/rocketfuel/1221/DelayLength.tsv -R -a lex-length-hops-suffix -i 1.0 -F
./build/Simulator -f data/datasets/rocketfuel/1221/DelayLengthWidth.tsv -R -a lex-length-width-suffix -i 1.0 -F
./build/Simulator -f data/datasets/rocketfuel/1221/DelayLengthWidth.tsv -R -a prod-width-length-hops-suffix -i 1.0 -F
./build/Simulator -f data/datasets/rocketfuel/1221/DelayHops.tsv -R -a lex-hops-suffix -i 1.0 -F