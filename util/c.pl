#!/usr/bin/perl

open(MOBF,"../lib/big.mob") || die "$!\n";
 
while(<MOBF>){
  chop;
  if(substr($_,0,1) eq '#'){
    $newobj = 1;
    printf "#%d\n",int(substr($_,1));
  } elsif(/^(\d+) +(\d+) +(\d+) +(\d+)d(\d+)\+(\d+) +(\d+)d(\d+)\+(\d+)$/){
    $lev = $1; $hr = $2; $ac = $3;
    $d1 = $4; $d2 = $5; $hp = $6;
    $d3 = $7; $d4 = $8; $dr = $9;
    $lev = 1 + ($lev + $ac + $hr)/10;
    $hr = $lev; $ac = $lev;
    $hp =  1 + $lev * $lev;
    $dr = $lev;
    $d1 = 1;
    $d2 = 1;
    $d3 = 1 + int(log(1+$lev));
    $d4 = 1 + int(sqrt($lev));
    printf "%d %d %d %dd%d+%d %dd%d+%d\n",
      $lev, $hr, $ac, $d1, $d2, $hp, $d3, $d4, $dr;
  } elsif(/^(\d+) +(\d+)$/){
    $gc = $1; $xp = $2;
    printf "%d %d\n",int(sqrt($gc)),1+$lev+int(sqrt($xp));
  } else {
    printf "%s\n", $_;
  }
}
