# RPBOB

```bash
find . -name "*.csv" -type f -delete
./run_simulations.sh -d bclique -u -N 5 --regen
./run_simulations.sh -d rocketfuel --as "1221" --regen -N 2
./run_simulations.sh -d rocketfuel   # uses default AS list
```