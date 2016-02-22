#!/usr/bin/perl

&do_help_file();

sub do_help_file {
  local($/);

  open(HF,"../lib/help_table") || die "$!\n";
  $chunk = <HF>;
  print length($chunk) , "\n";
  @a=split('#',$chunk);
  shift(@a);
  printf "  <select>\n";
  @klist = ();
  foreach $x (@a){
    s/^\n//;
    @b=split("\n",$x);
    shift(@b);
    $k = shift(@b);
    $d = join("\n",@b);
    if($k =~ /\"/){
      $k =~ s/\"//g;
      @kl = ($k);
    } else {
      @kl = split(' ',$k);
    }
    foreach $y (@kl){
      push(@klist,$y);
    }
  }
  foreach $x (sort @klist){
  printf "    <option>%s\n", $x;
  printf "  </select>\n";
}
