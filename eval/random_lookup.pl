#!/usr/bin/perl
use strict;
use warnings;

my $num_args = $#ARGV + 1;
if ($num_args < 1) {
  print "Invalid number of arguments!\n\nUsage: inputfile [inputfile ...]\n";
  exit;
}

my $PRINTALL = 0;
my %CountryDirections;
my %OutgoingDirections;
my %IncommingDirections;
my @ValuesPerLookup;
my @AnswerPerLookup;
my $lookupCounter = 0;
my $summaryoutputfile = "random_lookup_summary.csv";
my $summaryoutputfile_outgoing = "random_lookup_summary_out.csv";
my $summaryoutputfile_incomming = "random_lookup_summary_in.csv";
foreach (@ARGV) {
	parseFile($_);
}

open(OUT, ">$summaryoutputfile");
print OUT "Direction;Count;\n";
foreach ( sort {$CountryDirections{$b} <=> $CountryDirections{$a}} keys %CountryDirections ) {
	my $direction = $_;
	my $count = $CountryDirections{"$direction"};
	print OUT "$direction;$count;\n";
}
close(OUT);
open(OUT, ">$summaryoutputfile_outgoing");
print OUT "Source;OutgoingCount;\n";
foreach ( sort {$OutgoingDirections{$b} <=> $OutgoingDirections{$a}} keys %OutgoingDirections ) {
	my $source = $_;
	my $count = $OutgoingDirections{"$source"};
	print OUT "$source;$count;\n";
}
close(OUT);
open(OUT, ">$summaryoutputfile_incomming");
print OUT "Destination;IncommingCount;\n";
foreach ( sort {$IncommingDirections{$b} <=> $IncommingDirections{$a}} keys %IncommingDirections ) {
	my $destination = $_;
	my $count = $IncommingDirections{"$destination"};
	print OUT "$destination;$count;\n";
}
close(OUT);

my $answerCounter = 0;
my $valueCounter = 0;
foreach ( @AnswerPerLookup ) {
	$answerCounter += $_;
}
foreach ( @ValuesPerLookup ) {
	$valueCounter += $_;
}
print "Number of lookups: $lookupCounter\n";
print "Number of answers: $answerCounter\n";
print "Number of values : $valueCounter\n";
exit;

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
			$lookupCounter ++;
		}
		if ($line =~ m/DHT-EVALUATION VALUES_FOUND_EVENT HASH=/) {
			my @tokens = split(/\s+/, $line);
			my $sha1;
			my $eid;
			my $ip;
			my $port;
			my $sourceCountry;
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
				$sourceCountry = geoip("$ip");
			}
			$line = <IN>;
			while ( $line =~m/^---->/ ) {
				@tokens = split(/\s+/, $line);
				my $destinationCountry = geoip($tokens[1]);
				if (exists $CountryDirections{"$sourceCountry to $destinationCountry"}) {
					$CountryDirections{"$sourceCountry to $destinationCountry"}++;
				} else {
					$CountryDirections{"$sourceCountry to $destinationCountry"} = 1;
				}
				if (not $destinationCountry eq $sourceCountry ) {
					if(exists $OutgoingDirections{"$sourceCountry"}){
						$OutgoingDirections{"$sourceCountry"} ++;
					} else {
						$OutgoingDirections{"$sourceCountry"} = 1;
					}
					if(exists $IncommingDirections{"$destinationCountry"}){
						$IncommingDirections{"$destinationCountry"} ++;
					} else {
						$IncommingDirections{"$destinationCountry"} = 1;
					}
				}
				push(@value_ips, "$tokens[1]"."_"."$tokens[2]");
				$line = <IN>;
			}
		}
	}
	# Lese nächste Zeile aus der Logfile
	$line = <IN>;
}

close(IN);
if ($PRINTALL) {
open(OUT,">$outputfile");
print OUT "EID;NumberOfAnsweringNodes;NumberOfPossibleTargets\n";
}
foreach ( values %eids ) {
	my $eid = $_;
	my $count = $answers{"$eid"};
	my $value_count = $values{"$eid"};
	push(@ValuesPerLookup,"$value_count");
	push(@AnswerPerLookup,"$count");
	if ($PRINTALL) {
		print OUT "$eid;$count;$value_count\n";
	}
}
if ( not $PRINTALL) {
	return;
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
