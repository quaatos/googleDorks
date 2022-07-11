#!/usr/bin/perl

# Script de surveillance du raid, Jean-Edouard Babin, Juin 2007, v1.0

# Drive		    0        1        2        3         4
my @normal_state = ("Online","Online","Online","Online","Hotspare");
my $megacli = "/local/sbin/MegaCli";
my $o_msg = $msg = "Here is an error report !\n\n";
my $subject = "Raid error on ".`hostname`;

use Net::SMTP;

for(0..$#normal_state) {
	@result = `$megacli -pdInfo -PhysDrv [:$_] -a0`;
	foreach $line (@result) {
		if ($line =~ /Firmware state: (.*)/) {
			$msg .= "Phy. HD $_ is in state $1 instead of = $normal_state[$_]\n" if ($1 ne $normal_state[$_]);
		}
	}
}

@result = `$megacli -AdpBbuCmd -GetBbuStatus -a0`;
foreach $line (@result) {
	if ($line =~ /Fully Discharged: (.*)/) {
		$msg .= "Battery Fully Discharged is $1\n" if ($1 ne "No");	
	}
	if ($line =~ /isSOHGood: (.*)/) {
		$msg .= "Battery isSOHGood is $1\n" if ($1 ne "Yes");	

	}
}

if ($msg ne $o_msg) {
	$smtp = Net::SMTP->new('mail');
	$smtp->mail('bortzmeyer@nic.fr');
	$smtp->to('bortzmeyer@nic.fr');
	$smtp->data();
	$smtp->datasend("To: bortzmeyer@nic.fr\n");
	$smtp->datasend("Subject: $subject\n");
	$smtp->datasend("$msg\n");
	$smtp->dataend();
	$smtp->quit;
}

