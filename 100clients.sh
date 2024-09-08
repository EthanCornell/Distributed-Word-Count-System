#!/bin/bash
for i in {1..100}
do
  ./client 1722181920 &  # Adjust timestamp as needed
done
wait
