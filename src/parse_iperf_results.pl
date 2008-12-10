#!/usr/bin/perl
 
use strict;
use POSIX qw(ceil floor);

my @exps = ("simple", "hoolock", "hard");
my @topos = ("lossless", "lossy");
my @bws = ("50", "100", "200", "400", "800");

my $iters = 10;

my $path_base = "iperf_results";
my ($exp, $topo, $bw, $path);

foreach $topo (@topos) {
	print "============ Topology : $topo ===========\n\n";
	foreach $bw (@bws) {
		print "Bandwidth : $bw kbits/s\n";
		foreach $exp (@exps) {
			print "$exp : ";
			my (@pkt_loss, @xput, @out_of_order);
			for (my $i = 0; $i < $iters; $i++) {
				$path = "$path_base/$exp/$topo/$bw/out-$i.txt";
				if(!open(INFILE, "<$path")) {
					print "\n";
					last;
				}
				my @lines = <INFILE>;
				my $line_num = $#lines;
				if($lines[$line_num] =~ /out-of-order/) {
					chomp($lines[$line_num]);
					my @words = split(/\s+/, $lines[$line_num]);
					for(my $j = 1; $j <= $#words; $j++) {
						if($words[$j] eq "datagrams") {
							push(@out_of_order, $words[$j-1]);
							last;
						}
					}
					$line_num--;
				}
				else {
					push(@out_of_order, 0);
				}
				if(($line_num < 0) || !($lines[$line_num] =~ /%/)) {
					next;
				}
				chomp($lines[$line_num]);
				my @words = split(/\s+/, $lines[$line_num]);
				push(@xput, $words[$#words-1]*1470*8.0/(20*1000));
				push(@pkt_loss, substr($words[$#words-2], 0, -1));
			}
			my @sxput = sort {$a <=> $b} @xput;
			my @sloss = sort {$a <=> $b} @pkt_loss;
			my @sorder = sort {$a <=> $b} @out_of_order;
			print $sxput[$#sxput/2] . " " . $sloss[$#sloss/2] . " " . $sorder[$#sorder/2] . "\n";
			print "@xput\n";
			print "@pkt_loss\n";
			print "@out_of_order\n";
		}
		print "\n";
	}
	print "--------------------------------------\n\n";
}
