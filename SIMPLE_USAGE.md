解压种子文件，存放在临时目录
```shell
mkdir /tmp/PNG
tar -xvf ./data/png.tar.xz -C /tmp/PNG
```

裁剪
```shell
moonlight -d /tmp/PNG/png -n mypng
```

种子裁剪后的结果存放在json文件中
```shell
cat /tmp/PNG/png/mypng_solution.json
```

筛选出新的种子的副本
```shell
python python/moonseed.py /tmp/PNG/png/ /tmp/new_png/ /tmp/PNG/png/mypng_solution.json
```
