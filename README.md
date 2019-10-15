# file_service

## How to use?
```
git clone --git clone --recurse-submodules "LINK"
cd file_service
export LD_LIBRARY_PATH=$(pwd)/snappy-c
make clean
make
make server
make client

# To run server
./bin/server --state [SYNC|ASYNC] --n_sms [SEG_NUM] --sms_size [SEG_SIZE]

# To run client
./bin/client --conf ./input/sample.yaml --state [SYNC|ASYNC]
```

## Check the result of execution
The results are accumulated in ```./output/result.csv```. Please check the
performance of each execution.
