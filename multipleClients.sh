#!/bin/bash
for i in {1..20}
do
  ./client 1722181920 &  # Adjust the timestamp as needed
done
wait
