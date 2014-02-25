#!/usr/bin/perl
use strict;
use warnings;

my %options = (); 

sub usage
{
    print "-i <tree file> -o <symbols file>\n";
    exit;
}

sub init
{
    use Getopt::Std;
    getopts("i:o:", \%options) or usage();
    usage() if not ($options{o} and $options{i});
}

my %func_addr = ();

sub load_input
{
    my ($input_file) = @_; 
    print "Loading '$input_file'... ";
    open(INPUTFILE, "<$input_file") or die "Failed to open input file: '$input_file'\n";
    while (<INPUTFILE>)
    {
        my $line = $_;
        chomp $line;
        print "Processing line '$line'\n";
        if ($line =~ /(\d+)\s+0x(\d+)\s+(\d+)/)
        {
            my $addr = $2;
            print "Adding address $addr\n";
            if (exists $func_addr{$addr})
            {
                $func_addr{$addr}++;
            }
            else
            {
                $func_addr{$addr} = 1;
            }
        }   
    }   
    return;
}

sub dump_output
{
    my ($output_file) = @_; 
    open(OUTPUTFILE, ">$output_file") or die "Failed to open output file: '$output_file'\n";
    foreach my $addr (keys %func_addr)
    {
        print OUTPUTFILE "$addr $func_addr{$addr}\n"
    }
    close(OUTPUTFILE);
}

init();

load_input($options{i});
dump_output($options{o});
