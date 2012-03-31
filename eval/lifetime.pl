#!/usr/bin/perl
use strict;
use warnings;

my $num_args = $#ARGV + 1;
if ($num_args < 1) {
  print "Invalid number of arguments!\n\nUsage: inputfile [inputfile ...]\n";
  exit;
}

# Key is the sha1 of the announced string, value is the string
my %announcement;

# Key is sha1 of the announced string, value is the duration
my %announcement_duration;

my $announcement_output_file = "announcement_duration.csv";

foreach (@ARGV) {
	parseFile($_);
}

sub parseFile {
my $inputfile = $_[0];
#my $outputfile= $_[0] . ".csv";
open(IN, $inputfile);

my $line = <IN>;

while ($line)
{
	# Zeile gehört zum Output der LIB
	if ($line =~ m/DHT-EVALUATION/) {
		if ($line =~ m/START_ANNOUNCE EID=/) {
			my $eid;
			my $sha1;
			my $time;
			my @tokens = split(/\s+/, $line);
			foreach (@tokens) {
				if( $_ =~ m/^EID=/) {
					my @subtokens = split(/=/, $_);
					$eid = $subtokens[1];
				}
				if( $_ =~ m/^HASH=/) {
					my @subtokens = split(/=/, $_);
					$sha1 = $subtokens[1];
				}
				if($_ =~ m/^TIME=/) {
					my @subtokens = split(/=/, $_);
					$time = $subtokens[1];
				}
			}
			$announcement{"$sha1"} = "$eid";
			$announcement_duration{"$sha1"} = "$time";
		}
		
		if ($line =~ m/DHT-EVALUATION ANNOUNCE_DONE_EVENT HASH=/) {
			my $eid;
			my $sha1;
			my $time;
			my $duration;
			my @tokens = split(/\s+/, $line);
			foreach (@tokens) {
				if( $_ =~ m/^HASH=/) {
					my @subtokens = split(/=/, $_);
					$sha1 = $subtokens[1];
					$eid = $announcement{"$sha1"};
				}
				if($_ =~ m/^TIME=/) {
					my @subtokens = split(/=/, $_);
					$time = $subtokens[1];
				}
			}
			$duration = $time - $announcement_duration{"$sha1"};
			$announcement_duration{"$sha1"} = $duration;
		}
	}
	$line = <IN>;
}


close(IN);
#open(OUT,">$outputfile");

#close(OUT);
}#End parse File

open(OUT, ">$announcement_output_file");
print OUT "EID;HASH;DURATION\n";
foreach ( keys %announcement_duration ) {
	my $eid = $announcement{"$_"};
	my $duration = $announcement_duration{"$_"};
	print OUT "$eid;$_;$duration\n";
}

close(OUT);

