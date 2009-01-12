#!/usr/bin/perl
 
use strict;
use POSIX qw(ceil floor);
use Statistics::Descriptive;

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
			print "$exp : \n";
			my (@pkt_loss, @xput, @out_of_order);
			my $xstats = Statistics::Descriptive::Full->new();
			my $lstats = Statistics::Descriptive::Full->new();
			my $ostats = Statistics::Descriptive::Full->new();
			for (my $i = 0; $i < $iters; $i++) {
				$path = "$path_base/$exp/$topo/$bw/out-$i.txt";
				if(!open(INFILE, "<$path")) {
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
			$xstats->add_data(@xput);
			my $xci = 1.96 * $xstats->standard_deviation()/sqrt($iters);
			#print sprintf("%.2f",$xstats->mean()) . " +/- " . sprintf("%.2f",$xci) . " : @xput\n";
			$lstats->add_data(@pkt_loss);
			my $lci = 1.96 * $lstats->standard_deviation()/sqrt($iters);
			print sprintf("%.2f",$lstats->mean()) . " +/- " . sprintf("%.2f",$lci) . "\n";#" : @pkt_loss\n";
			$ostats->add_data(@out_of_order);
			my $oci = 1.96 * $ostats->standard_deviation()/sqrt($iters);
			#print sprintf("%.2f",$ostats->mean()) . " +/- " . sprintf("%.2f",$oci) . " : @out_of_order\n";
		}
		print "\n";
	}
	print "--------------------------------------\n\n";
}
