#!/bin/bash
g++ -o ../../bin/producer Producer.cpp -g -O2 -m64 -I../../include -I../../src/kpr -L../../bin -lrocketmq64 
g++ -o ../../bin/pushconsumer PushConsumer.cpp -g -O2 -m64 -I../../include -I../../src/kpr/ -L../../bin -lrocketmq64 
