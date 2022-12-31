#!/usr/bin/env perl
# -----------------------------------------------------------------------------
# MIT License
#
# Copyright 2022-Present Miguel A. Guerrero
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal # in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
# -----------------------------------------------------------------------------

use Getopt::Long;
use strict;

my $ver="1.0";
my $cpu_freq_hz=1.0e9;
my $opt_top="top";
my $opt_in="trace_file.log";
my $opt_out="events.vcd";
my $opt_gz=0;
my $opt_event_ps=100;
my $help=0;

if (!&GetOptions(
      "top=s" => \$opt_top,
      "in=s" => \$opt_in,
      "out=s" => \$opt_out,
      "event_ps=n" => \$opt_event_ps,
      "gz!" => \$opt_gz, 
      "help!" => \$help
   ) || $help) 
{
   &syntax;
}


my %func_name;
my $symb_index=0;
my $symb_base = 64;
my $symb_range = 26;
my $vars_in_file;
my %initial;
my %gvars_sym;
my %type_of;
my @lines;

open(FIN, "$opt_in") || die "ERROR: cannot read from $opt_in";
while(<FIN>) {

   my $sav_line = $_;
   s/^\s+//;
   s/\s+$//;

   if (/^$/) { next; }

   my ($time, $cmd, @event) = split(/\s+/, $_);
   my $event = join(" ", @event);

   if ($cmd eq "ON" || $cmd eq "OFF") {
      my $sym = get_symbol($event);
      push @lines, "$sym $sav_line";
      $type_of{$sym} = "bit";
   }
   elsif ($cmd eq "EVENT") {
      my $sym = get_symbol($event);
      push @lines, "$sym $sav_line";
      $type_of{$sym} = "event";
   }
   elsif ($cmd =~ /.*_VAR$/) { #value assignement
      my ($var, $val) = split(/\s+/, $event);
      my $sym = get_symbol($var);
      if ($cmd eq "PUSH_VAR") {
         $type_of{$sym} = $val;
      }
      push @lines, "$sym $sav_line";
   }
   elsif ($cmd eq "FREQ_IN_HZ") {
       $cpu_freq_hz = int($event)
   }
}
close (FIN);

my $cpu_cycle_ps=1.0e12/$cpu_freq_hz;

my $date=`date`; chomp $date;
if ($opt_gz) {
   open(VCD, "| gzip > $opt_out.gz") || die "ERROR: cannot open $opt_out.gz";
}
else {
   open(VCD, "> $opt_out") || die "ERROR: cannot open $opt_out";
}
print_hdr($date, "1ps");
print_scopes($opt_top);
print_dumpvars_initial();
print_value_changes();

close VCD;

exit 0;

#--------------------------------------------------------------------
#functions
#--------------------------------------------------------------------
sub vcd_type_of {
  my ($type) = @_;
  $type =~ s/unsigned//;
  $type =~ s/signed//;
  $type =~ s/int//;
  $type =~ s/\s+/ /g;
  $type =~ s/^\s+//;
  $type =~ s/\s+$//;
  if ($type eq "long long") { return "reg 64"; }
  if ($type eq "long") { return "reg 32"; }
  if ($type eq "short") { return "reg 16"; }
  if ($type eq "char") { return "reg 16"; }
  if ($type eq "float") { return "reg 32"; }
  if ($type eq "double") { return "reg 64"; }
  if ($type eq "bit") { return "reg 1"; }
  if ($type eq "event") { return "reg 1"; }
  return "reg 32";
}

sub next_symb {
   $symb_index++;
   my $x=$symb_index;
   my $res="";
   do {
      my $c = sprintf("%c", $symb_base + $x % $symb_range);
      $res = $c . $res;
      $x = int ($x / $symb_range);
   } while ($x != 0);
   return $res;
}

sub get_symbol {
   my ($key) = @_;

   if (!$gvars_sym{$key}) {
      $gvars_sym{$key}=next_symb();
      $vars_in_file.="$key,";
   }

   return $gvars_sym{$key};
}

#--------------------------------------------------------------------
# Print VCD file header
#--------------------------------------------------------------------
sub print_hdr {
   my ($date, $timescale) = @_;
   print VCD <<EOM
\$date
  $date
\$end
\$version
  trace2vcd $ver
\$end
\$timescale
  $timescale
\$end
EOM
;
}

#--------------------------------------------------------------------
# Sanitize top and variable names for VCD
#--------------------------------------------------------------------
sub sanitize_name {
   my ($name) = @_;
   $name =~ s/\./_/g;
   $name =~ s/, /,/g;
   $name =~ s/</_/g;
   $name =~ s/>/_/g;
   $name =~ s/:/_/g;
   $name =~ s/ +/_/g;
   return $name;
}   

#--------------------------------------------------------------------
# Print scopes (only top for now) and vars per scope
#--------------------------------------------------------------------
sub print_scopes {
   my ($top) = @_;
   my $disp_top = $top;
   $disp_top =~ s/.*\///;
   $disp_top = sanitize_name($disp_top);
   print VCD "\$scope module $disp_top \$end\n";

   $vars_in_file =~ s/,$//;
   my @vars_in_file=split(/,/, $vars_in_file);
   foreach my $var (@vars_in_file) {
      my $sym = get_symbol($var);
      my $typ = $type_of{$sym};
      $initial{$sym}= $typ eq "bit" || $typ eq "event" ? "b0" : "bx";
      my $disp_var = sanitize_name($var);
      my $type = vcd_type_of($typ);
      print VCD "\$var $type $sym $disp_var \$end\n";
   }

   print VCD "\$upscope \$end\n";
   print VCD "\$enddefinitions \$end\n";
}

#--------------------------------------------------------------------
# print initial variable values
#--------------------------------------------------------------------
sub print_dumpvars_initial {
   print VCD "\$dumpvars\n";
   foreach my $varname (sort keys %gvars_sym) {
      my $sym=$gvars_sym{$varname};
      print VCD "$initial{$sym} $sym\n";
   }
   print VCD "\$end\n";
}

#--------------------------------------------------------------------
# Print a value depending on type
#--------------------------------------------------------------------
sub print_sub_value {
   my ($cmd, $sym, @func) = @_;
   if ($cmd eq "ON") {
      printf VCD "b1 $sym\n";
   }
   elsif ($cmd eq "OFF") {
      printf VCD "b0 $sym\n";
   }
   elsif ($cmd eq "TRACE_VAR") {
      my ($var, $val) = split(/ /, "@func");
      printf VCD "b%b $sym\n", $val;
   }
   elsif ($cmd eq "POP_VAR") {
      printf VCD "bx $sym\n";
   }
}

#--------------------------------------------------------------------
# Main loop to print all VCD value changes
#--------------------------------------------------------------------

sub escale_time {
   my ($time) = @_;
   my $escaled_time = int($cpu_cycle_ps * $time);
   return $escaled_time;
}


my $pescaled_time=-1;

sub emit_time {
   my ($escaled_time) = @_;
   if ($escaled_time != $pescaled_time) {
       print VCD "#$escaled_time\n";
       $pescaled_time = $escaled_time;
   }
}

sub print_value_changes {
   my @pending_sym;
   my @pending_time;
   foreach my $line (@lines) {
      my ($sym, $clks, $cmd, @func) = split(/\s+/, $line);
      my $time = &escale_time($clks);

      while ($#pending_time != -1 && $pending_time[0] < $time) {
         emit_time($pending_time[0]);
         print_sub_value("OFF", $pending_sym[0]);
         shift @pending_sym;
         shift @pending_time;
      }

      emit_time($time);

      if ($cmd eq "EVENT") {
          print_sub_value("ON", $sym);
          push @pending_time, $time + $opt_event_ps;
          push @pending_sym, $sym;
      }
      else {
          print_sub_value($cmd, $sym, @func);
      }

   }
}

#--------------------------------------------------------------------
#  Display syntax information
#--------------------------------------------------------------------
sub syntax {
   print<<EOF

    Usage: $0 [options] file_in 

    Where options is any combination of the following:

       -top <str>      Specifies executable path (default '$opt_top')
       -in <path>      Input file name. (default '$opt_in')
       -out <path>     Output file name. No gz required if using -gz  (default '$opt_out')
       -event_ps <int> Duration/width used to display events in ps (default $opt_event_ps)
       -[no]gz         gzip output file while dumping (default $opt_gz)
       -help           Display this message

EOF
;
   exit(1);
}
