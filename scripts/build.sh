#! /bin/bash

echo "=================================开始构建================================="
if [ -d "../output/" ];then
  rm -r ../output
fi
mkdir ../output
mkdir ../output/bin
mkdir ../output/config
cp ../config/config.json ../output/config

if [ -d "../tmp/" ];then
  rm -r ../tmp
fi
mkdir ../tmp
cd ../tmp
cmake -DCMAKE_BUILD_TYPE=Release ..
make
mv ./LiveStreamServer ../output/bin
cd ..
rm -r ./tmp
echo "=================================构建完成================================="
