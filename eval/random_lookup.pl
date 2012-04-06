#!/usr/bin/perl
use strict;
use warnings;

my $num_args = $#ARGV + 1;
if ($num_args < 1) {
  print "Invalid number of arguments!\n\nUsage: inputfile [inputfile ...]\n";
  exit;
}

foreach (@ARGV) {
	parseFile($_);
}

sub parseFile {
my $inputfile = $_[0];
my $outputfile= $_[0] . ".csv";
open(IN, $inputfile);

#  key: EID  value: Anzahl der antwortenden IP's
my %answers;
#  IP_PORT list
my @answering_ips;
# IP_PORT list
my @value_ips;
#  key: EID  value: Anzahl der empfangenen IP's
my %values;
#  key: SHA1 value: EID
my %eids;
my $line = <IN>;

while ($line)
{
	# Zeile gehört zum Output der LIB
	if ($line =~ m/DHT-EVALUATION/) {
		#Start eines Requests:
		if ($line =~ m/START_LOOKUP EID=dtn:\/\/random_/) {
			my $eid;
			my $sha1;
			my @tokens = split(/\s+/, $line);
			foreach (@tokens) {
				if( $_ =~ m/^EID=/) {
					my @subtokens = split(/=/, $_);
					$eid = $subtokens[1];
					$answers{"$eid"} = 0;
					$values{"$eid"} = 0;
				}
				if( $_ =~ m/^HASH=/) {
					my @subtokens = split(/=/, $_);
					$sha1 = $subtokens[1];
				}
			}
			# Speicher den Hash und die EID ab:
			$eids{"$sha1"} = "$eid";
		}
		if ($line =~ m/DHT-EVALUATION VALUES_FOUND_EVENT HASH=/) {
			my @tokens = split(/\s+/, $line);
			my $sha1;
			my $eid;
			my $ip;
			my $port;
			foreach (@tokens) {
				if( $_ =~ m/^HASH=/) {
					my @subtokens = split(/=/, $_);
					$sha1 = $subtokens[1];
					$eid = $eids{"$sha1"};
					if (exists $answers{"$eid"}) {
						$answers{"$eid"} = $answers{"$eid"} + 1;
					} else {
						$answers{"$eid"} = 1;
					}
				}
				if( $_ =~ m/^COUNT=/) {
					my @subtokens = split(/=/, $_);
					if(exists $values{"$eid"}) {
						$values{"$eid"} = $values{"$eid"} + $subtokens[1];
					}
				}
				if( $_ =~ m/^FROM=/) {
					my @subtokens = split(/=/, $_);
					$ip = $subtokens[1];
				}
				if( $_ =~ m/^PORT=/) {
					my @subtokens = split(/=/, $_);
					$port = $subtokens[1];
				}
			}
			if( $ip ) {
				push(@answering_ips,"$ip". "_" ."$port");
			}
			$line = <IN>;
			while ( $line =~m/^---->/ ) {
				@tokens = split(/\s+/, $line);
				push(@value_ips, "$tokens[1]"."_"."$tokens[2]");
				$line = <IN>;
			}
		}
	}
	# Lese nächste Zeile aus der Logfile
	$line = <IN>;
}

close(IN);
open(OUT,">$outputfile");

print OUT "EID;NumberOfAnsweringNodes;NumberOfPossibleTargets\n";
foreach ( values %eids ) {
	my $eid = $_;
	my $count = $answers{"$eid"};
	my $value_count = $values{"$eid"};
	print OUT "$eid;$count;$value_count\n";
}

print OUT "\n---------------------------------------\n\n";
print OUT "AnsweringIP;Port;Country\n";
foreach (@answering_ips) {
	my @tokens = split(/_/, $_ );
	my $ip = $tokens[0];
	my $port = $tokens[1];
	my $country = geoip("$ip");
	print OUT "$ip;$port;$country\n";
}
print OUT "\n---------------------------------------\n\n";
print OUT "PossibleTarget;Port;Country\n";
foreach (@value_ips) {
	my @tokens = split(/_/, $_ );
	my $ip = $tokens[0];
	my $port = $tokens[1];
	my $country = geoip("$ip");
	print OUT "$ip;$port;$country\n";
}
close(OUT);
}#End parse File

sub geoip
{
	my $country = `geoiplookup $_[0] 2>/dev/null`;
	$country = substr($country, index($country, ':') + 2 );
	$country =~ s/\n//; 
	return $country;
}
