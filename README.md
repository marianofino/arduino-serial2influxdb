# Arduino Serial2InfluxDB

A simple Linux program to store Arduino data from serial to InfluxDB. 

## Compilation

```
$ gcc arduino-serial2influxdb.c -o arduino-serial2influxdb -lcurl
```

## Basic usage

Run:

```
$ ./arduino-serial2influxdb -h
```

Example:

```
$ ./arduino-serial2influxdb -p /dev/ttyACM0 -m temperature,floor=first,orientation=north -u http://localhost:8086/write?db=mydb
```
