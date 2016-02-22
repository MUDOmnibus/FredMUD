#!/usr/bin/perl

open(OBJF,"../lib/big.obj") || die "$!\n";
 
while(<OBJF>){
  chop;
  if(substr($_,0,1) eq '#'){
    $newobj = 1;
    printf "#%d\n",int(substr($_,1));
  } elsif($_ eq "A"){
    $affect = 1;
    print "A\n";
  } elsif(/^(\d+) +(\d+) +(\d+)$/){
    $w = $1; $x = $2; $y = $3;
    if($newobj){
      $type = $w;
      $newobj = 0;
      printf "%s\n", $_;
    } else {
      $z/=2;
      $y = int(sqrt($y));
      printf "%d %d %d\n", $w, $x, $y;
    }
  } elsif(/^(\d+) +(\d+) +(\d+) +(\d+)$/){
    $w = $1; $x = $2; $y = $3; $z = $4;
    if($type == 9){
      $w = (1 + $w)/2;
    } elsif($type==5){
      $x = (1 + $x)/2;
      $y = (1 + $y)/2;
    }
    printf "%d %d %d %d\n", $w, $x, $y, $z;
  } elsif(/^(\d+) +(\d+)$/){
    $w = $1; $x = $2;
    if($affect){
      $affect = 0;
      $x = 1 + int(sqrt($x));
      printf "%d %d\n",$w,$x;
    } else {
      printf "%s\n", $);
    }
  } elsif(/^(\d+) +\-(\d+)$/){
    $w = $1; $x = $2;
    if($affect){
      $affect = 0;
      $x = -1 - int(sqrt($x));
      printf "%d %d\n",$w,$x;
    } else {
      printf "%s\n", $);
    }
  } else {
    printf "%s\n", $_;
  }
}
