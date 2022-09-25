# MP1 - Introduction to Linux Kernel Programming
##### Instructor: Tianyin Xu
Deadline --- Tuesday, September, 27th, 11:59PM

Create a Linux Kernel Module (LKM) that measures the Userspace CPU Time of processes registered within the kernel module

## Project Setup
This project depends on the installation of the following essenstial packages:

None

## Build Project
With the project setup, build the command-line application as follow:
```
make
```

To clean up the project
```
make clean
```

## Test Linux Kernel Module
#### 1. Test with 1 userapp
Run the following command
```
test_kernel_module_1.sh
```

#### 2. Test with 2 userapp
```
test_kernel_module_2.sh
```

## Design

#### Location of the log collected on VMs
* The log for `3 nodes, 2Hz each, running for 100s` is located in the folder `Event_Logging_Analysis/ThreeNode_Log`
* The log for `8 nodes, 5Hz each, running for 100s` is located in the folder `Event_Logging_Analysis/EightNode_Log`

#### Plot TimeDelay & Bandwidth vs Time for Three Nodes
The following python file is used to create the plots for Three-Node Log
* ThreeNode_TimeDelay_DataLength_Node_CentralizedLogger.py

To plot and save the figures for Three-Node Log

```
python ThreeNode_TimeDelay_Bandwith.py -n <Node#> -n1 <Node1LogPath> -n2 <Node2LogPath> -n3 <Node3LogPath>
```
`<Node#>` Total number of nodes [`3`in this case]

`<Node1LogPath>` Path to node 1 Log [node1.csv file]

`<Node2LogPath>` Path to node 2 Log [node2.csv file]

`<Node3LogPath>` Path to node 3 Log [node3.csv file]

Example: `<Node1LogPath>` for `Three-Node Log` will be
```
Event_Logging_Analysis/ThreeNode_Log/node1.csv
```

#### Plot TimeDelay & Bandwidth vs Time for Eight Nodes
The following python file is used to create the plots for Eight-Node Log
* EightNode_TimeDelay_DataLength_Node_CentralizedLogger.py

To plot and save the figures for Eight-Node Log

Please type the following commands in the `SAME` row, new line is added for `display purpose`.

```
python ThreeNode_TimeDelay_Bandwith.py -n <Node#> -n1 <Node1LogPath> -n2 <Node2LogPath> -n3 <Node3LogPath> -n4 <Node4LogPath> -n5 <Node5LogPath> -n6 <Node6LogPath> -n7 <Node7LogPath> -n8 <Node8LogPath>
```
`<Node#>` Total number of nodes [`8`in this case]

`<Node1LogPath>` Path to node 1 log [node1.csv file]

`<Node2LogPath>` Path to node 2 log [node2.csv file]

`<Node3LogPath>` Path to node 3 log [node3.csv file]

`<Node4LogPath>` Path to node 4 log [node4.csv file]

`<Node5LogPath>` Path to node 5 log [node5.csv file]

`<Node6LogPath>` Path to node 6 log [node6.csv file]

`<Node7LogPath>` Path to node 7 log [node7.csv file]

`<Node8LogPath>` Path to node 8 log [node8.csv file]

Example: `<Node1LogPath>` for `Eight-Node Log` will be
```
Event_Logging_Analysis/EightNode_Log/node1.csv
```

#### TimeDelay & Bandwidth vs Time Plots
The plots for `Three-Node Log` are saved in the following folders
* Event_Logging_Analysis/ThreeNode_TimeDelay_Plot
* Event_Logging_Analysis/ThreeNode_Bandwidth_Plot

The plots for `Eight-Node Log` are saved in the following folders
* Event_Logging_Analysis/EightNode_TimeDelay_Plot
* Event_Logging_Analysis/EightNode_Bandwidth_Plot

Each `Event_Logging_Analysis/X_TimeDelay_Plot` folder contains the following graphs
* X_Minimum_Time_Delay_vs_Time
* X_Maximum_Time_Delay_vs_Time
* X_Median_Time_Delay_vs_Time
* X_Ninety_Percentile_Time_Delay_vs_Time

Each `Event_Logging_Analysis/X_Bandwidth_Plot` folder contains the following graph
* X_AverageBandwidth_vs_Time

## Developers
* Hongbo Zheng [NetID: hongboz2]
