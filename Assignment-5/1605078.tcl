# simulator
set ns [new Simulator]


# ======================================================================
# Define options

set val(chan)         Channel/WirelessChannel  ;# channel type
set val(prop)         Propagation/TwoRayGround ;# radio-propagation model
set val(ant)          Antenna/OmniAntenna      ;# Antenna type
set val(ll)           LL                       ;# Link layer type
set val(ifq)          Queue/DropTail/PriQueue  ;# Interface queue type
set val(ifqlen)       50                       ;# max packet in ifq
set val(netif)        Phy/WirelessPhy          ;# network interface type
set val(mac)          Mac/802_11               ;# MAC type
set val(rp)           AODV                     ;# ad-hoc routing protocol 
set val(nn)           20                       ;# number of mobilenodes
# =======================================================================

# =======================================================================
#Define Varying Options
set height(1)       250
set width(1)        250

set height(2)       500
set width(2)        500

set height(3)       750
set width(3)        750

set height(4)       1000
set width(4)        1000

set height(5)       1250
set width(5)        1250

set nodeCount(1)    20
set nodeCount(2)    40
set nodeCount(3)    60
set nodeCount(4)    80
set nodeCount(5)    100

set flowCount(1)    10
set flowCount(2)    20
set flowCount(3)    30
set flowCount(4)    40
set flowCount(5)    50

set height(0)       500
set width(0)        500
set nodeCount(0)    10
set flowCount(0)    5

# =======================================================================

# Setting varying Options
set op              1
set baseLineOp      2
set val(nn)         $nodeCount($baseLineOp)                 ;# number of nodes
set val(nf)         $flowCount($baseLineOp)                         ;# number of flows
set val(height)     $height($op)
set val(width)      $width($op)
set output_file     output_flow.txt

# trace file
set trace_file [open trace.tr w]
$ns trace-all $trace_file

# nam file
set nam_file [open animation.nam w]
$ns namtrace-all-wireless $nam_file $val(height) $val(width)

# topology: to keep track of node movements
set topo [new Topography]
$topo load_flatgrid $val(height) $val(width) ;# 500m x 500m area

# general operation director for mobilenodes
create-god $val(nn)


# node configs
# ======================================================================

# $ns node-config -addressingType flat or hierarchical or expanded
#                  -adhocRouting   DSDV or DSR or TORA
#                  -llType	   LL
#                  -macType	   Mac/802_11
#                  -propType	   "Propagation/TwoRayGround"
#                  -ifqType	   "Queue/DropTail/PriQueue"
#                  -ifqLen	   50
#                  -phyType	   "Phy/WirelessPhy"
#                  -antType	   "Antenna/OmniAntenna"
#                  -channelType    "Channel/WirelessChannel"
#                  -topoInstance   $topo
#                  -energyModel    "EnergyModel"
#                  -initialEnergy  (in Joules)
#                  -rxPower        (in W)
#                  -txPower        (in W)
#                  -agentTrace     ON or OFF
#                  -routerTrace    ON or OFF
#                  -macTrace       ON or OFF
#                  -movementTrace  ON or OFF

# ======================================================================

$ns node-config -adhocRouting $val(rp) \
                -llType $val(ll) \
                -macType $val(mac) \
                -ifqType $val(ifq) \
                -ifqLen $val(ifqlen) \
                -antType $val(ant) \
                -propType $val(prop) \
                -phyType $val(netif) \
                -topoInstance $topo \
                -channelType $val(chan) \
                -agentTrace ON \
                -routerTrace ON \
                -macTrace OFF \
                -movementTrace OFF


# random number generator
proc randGenerator {min max} {
    expr {int(rand() * ($max - $min)) + $min}
}

# create nodes
# set l [expr $val(nn) < 50 ? 5: 10]
set l 5
set node(limit) [expr $val(nn) / $l]
set node(h_dist) [expr ($val(width) / $node(limit))]
set node(v_dist) [expr ($val(height) * $node(limit)) / $val(nn)]

set rowCount 0
set colCount 0
for {set i 0} {$i < $val(nn) } {incr i} {
    set node($i) [$ns node]
    $node($i) random-motion 0       ;# disable random motion

    # $node($i) set X_ [expr (500 * $i) / $val(nn)]
    # $node($i) set Y_ [expr (500 * $i) / $val(nn)]
    # $node($i) set Z_ 0

    $node($i) set X_ [expr ($colCount * $node(h_dist))]
    $node($i) set Y_ [expr ($rowCount * $node(v_dist))]
    $node($i) set Z_ 0

    set x_dest [randGenerator 1 $val(width)]
    set y_dest [randGenerator 1 $val(height)]
    set z_dest 0
    set speed 3.0
    # puts "x_dest: $x_dest y_dest: $y_dest speed: $speed"
    $ns at 1.0 "$node($i) setdest $x_dest $y_dest $speed"

    set colCount [expr ($colCount + 1)]
    if {$colCount == $node(limit)} {
        set rowCount [expr ($rowCount + 1)]
    }
    set colCount [expr ($colCount % $node(limit))]

    $ns initial_node_pos $node($i) 20
} 


# setting the src
set src [randGenerator 0 $val(nn)]
# puts "src: $src"


# random destionation
for {set i 0} {$i < $val(nf)} {incr i} {
    set dest [randGenerator 0 $val(nn)]
    while {$src == $dest} {
        set dest [randGenerator 0 $val(nn)]
    }
    # puts "flow: $src --- $dest"


    set udp [new Agent/UDP]
    $ns attach-agent $node($src) $udp
    
    set null [new Agent/Null]
    $ns attach-agent $node($dest) $null
    
    # connect agents
    $ns connect $udp $null
    $udp set fid_ $i

    # Traffic generator
    set cbr [new Application/Traffic/CBR]
    $cbr attach-agent $udp
    $cbr set type_ CBR
    $cbr set packet_size_ 1000
    $cbr set rate_ 16kb
    $cbr set random_ false

    # start traffic generation
    $ns at 1.0 "$cbr start"
}



# End Simulation

# Stop nodes
for {set i 0} {$i < $val(nn)} {incr i} {
    $ns at 30.0 "$node($i) reset"
}

# call final function
proc finish {} {
    global ns trace_file nam_file
    $ns flush-trace
    close $trace_file
    close $nam_file
}

proc halt_simulation {} {
    global ns output_file
    puts "Simulation ending"
    exec awk -f parse.awk trace.tr >> $output_file
    $ns halt
}

$ns at 30.0001 "finish"
$ns at 30.0002 "halt_simulation"




# Run simulation
puts "Simulation starting"
$ns run

