# ompi-mutual-friends


```
$ mkdir build && cd build
$ cmake ../
$ make

$ docker run --network host --name my-memcache -d memcached
$ MEMCACHED_SERVER=localhost ./friends ../data/twitter_combined.txt

$ cat output.txt | grep "[17711130, 99712896]"
[17711130, 99712896]: 21447363 22462180 34428380 36475508 43003845 107512718 126391227 222851879 223990701
```
