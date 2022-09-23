export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/home/xfrancv/Work/ConsistentSurrogate/code/ecml2022/manet/adag_solver"

python3 train.py test hmc_30x30x100_mis0 0
python3 eval.py test hmc_30x30x100_mis0 1000 0