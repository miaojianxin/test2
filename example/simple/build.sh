#!/bin/bash
g++ -o ../../bin/producer Producer.cpp -g -m64 -I../../include -L../../bin -lrocketmq64 

g++ -o ../../bin/pullconsumer PullConsumer.cpp -m64 -g -I../../include -L../../bin -lrocketmq64 

g++ -o ../../bin/pushconsumer PushConsumer.cpp -m64 -g -I../../include -L../../bin -lrocketmq64 


g++ -o ../../bin/producer_alimq  producer_alimq.cpp -g -m64  -I../../include -L../../bin  -lpthread -ldl  -laliyunmq64

g++ -o ../../bin/pushconsumer_alimq  pushconsumer_alimq.cpp -g -m64  -I../../include -L../../bin  -lpthread -ldl -laliyunmq64

g++ -o ../../bin/produceroneway_alimq  produceroneway_alimq.cpp -g -m64  -I../../include -L../../bin   -lpthread -ldl  -laliyunmq64

g++ -o ../../bin/pushconsumer_alimq  pushconsumer_alimq.cpp -g -m64  -I../../include -L../../bin  -lpthread -ldl  -laliyunmq64

g++ -o ../../bin/pullconsumer_alimq  pullconsumer_alimq.cpp -g -m64  -I../../include -L../../bin  -lpthread -ldl -laliyunmq64
 
