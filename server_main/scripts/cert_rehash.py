#!/usr/bin/perl
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#
use File::Copy;


# Perl c_rehash script, scan all files in a directory
# and add symbolic links to their hash values.

my $openssl;
my $tmpdir = "/tmp/userfiles";
my $prefix = "/usr/local";
my $trustedStore_dir = "${IMA_SVR_DATA_PATH}/data/certificates/truststore";

open FILE, ">/tmp/imaserver_cert_rehash.log";

#action can be add, delete, overwrite. The default is add.
my $action = "add";

if(defined $ENV{OPENSSL}) {
        $openssl = $ENV{OPENSSL};
} else {
        $openssl = "openssl";
        $ENV{OPENSSL} = $openssl;
}

my $pwd;
eval "require Cwd";
if (defined(&Cwd::getcwd)) {
        $pwd=Cwd::getcwd();
} else {
        $pwd=`pwd`; chomp($pwd);
}
my $path_delim = ($pwd =~ /^[a-z]\:/i) ? ';' : ':'; # DOS/Win32 or Unix delimiter?

$ENV{PATH} = "$prefix/bin" . ($ENV{PATH} ? $path_delim . $ENV{PATH} : ""); # prefix our path

if(! -x $openssl) {
        my $found = 0;
        foreach (split /$path_delim/, $ENV{PATH}) {
                if(-x "$_/$openssl") {
                        $found = 1;
                        $openssl = "$_/$openssl";
                        last;
                }
        }
        if($found == 0) {
                print FILE "ima_rehash: rehashing skipped ('openssl' program not available)\n";
                # system("${IMA_SVR_INSTALL_PATH}/bin/icu_gettext", "CWLNA8048", "ERROR", "openssl");
                exit 0;
        }
}

$numArgs = $#ARGV + 1;

print FILE "argument is $numArgs\n";
if ($numArgs != 3) {
    print FILE "wrong input arguments\n";
    exit 1;
}
my $action = $ARGV[0];
my $profilename = $ARGV[1];
my $certname = $ARGV[2];

print FILE "argument 1 is $profilename\n";
print FILE "argument 2 is $certname\n";
print FILE "argument 3 is $action\n";

my $profile_dir = "${trustedStore_dir}/${profilename}";
my $profilecert_dir = "${profile_dir}_capath";
my $profile_cafile = "${profilename}_cafile.pem";
print FILE "profile dirrectory is $profile_dir\n";

if (-d $profile_dir) {
    print FILE "$profile_dir is exist\n";
    if (-e "${profile_dir}/${certname}") {
        if (lc($action) eq "overwrite") {
            copy("$tmpdir/$certname", "$profile_dir/$certname") or die "Copy failed: $!";
        } elsif (lc($action) eq "delete"){
            my ($rm_ret) = remove_cert($certname);
            if ($rm_ret == 1) {
                exit 1;
            }
            exit 0;
        }else {
            # means action is add.
            print FILE "$certname already exists, please Set Overwrite=True to replace the existing certificate.\n";
            #system("${IMA_SVR_INSTALL_PATH}/bin/icu_gettext", "CWLNA8280", "ERROR");
            exit 1;
        }
    } else {
        if (lc($action) eq "delete"){
            #system("${IMA_SVR_INSTALL_PATH}/bin/icu_gettext", "CWLNA8048", "ERROR", $certname);
            print FILE "$certname does not exist.\n";
            exit 1;
        } else {
            copy("$tmpdir/$certname", "$profile_dir/$certname") or die "Copy failedd: $!";
        }
    }
} else {
    if (lc($action) eq "delete"){
       print FILE "There is no certificate currently associated with the $profilename.\n";
       #system("${IMA_SVR_INSTALL_PATH}/bin/icu_gettext", "CWLNA8540", "INFO", "$profilename");
       exit 1;
    }
    print FILE "$profile_dir is not exist\n";
    mkdir($profile_dir, 0700)  or die "mkdir failed: $!";
    if (-e "$tmpdir/$certname") {
       copy("$tmpdir/$certname", "$profile_dir/$certname") or die "Copy failed: $!";
    } else {
       print FILE "WARNING: $tmpdir/$certname does not contain a certificate or CRL: skipping\n";
       #system("${IMA_SVR_INSTALL_PATH}/bin/icu_gettext", "CWLNA8468", "ERROR", "Certificate", "$certname");
       exit 1;
   }
}

if (!-d $profilecert_dir) {
    mkdir($profilecert_dir, 0700)  or die "mkdir failed: $!";
}

# Check to see if certificates and/or CRLs present.
my ($cert, $crl) = check_file("$profile_dir/$certname");
if(!$cert && !$crl) {
    print FILE "WARNING: $certname does not contain a certificate or CRL: skipping\n";
    #system("${IMA_SVR_INSTALL_PATH}/bin/icu_gettext", "CWLNA8229", "ERROR");
    exit 1;
}

chdir "$profilecert_dir" or die "change dir failed: $!";
link_hash_cert("$profile_dir/$certname") if($cert);
my ($regen_ret) = regen_cafile();
if ($regen_ret == 1) {
   exit 1;
}

exit 0;

sub hash_dir {
        my %hashlist;
        print "Doing $_[0]\n";
        chdir $_[0];
        opendir(DIR, ".");
        my @flist = readdir(DIR);
        # Delete any existing symbolic links
        foreach (grep {/^[\da-f]+\.r{0,1}\d+$/} @flist) {
                if(-l $_) {
                        unlink $_;
                }
        }
        closedir DIR;
        FILE: foreach $fname (grep {/\.pem$/} @flist) {
                # Check to see if certificates and/or CRLs present.
                my ($cert, $crl) = check_file($fname);
                if(!$cert && !$crl) {
                        print FILE "WARNING: $fname does not contain a certificate or CRL: skipping\n";
                        #system("/bin/icu_gettext", "CWLNA8229", "ERROR");
                        next;
                }
                link_hash_cert($fname) if($cert);
                link_hash_crl($fname) if($crl);
        }
}

sub remove_cert {
    my $fname = $_[0];

    print "certname $_[0]\n";
    unlink "$profile_dir/$fname";

    if (!-d $profilecert_dir) {
        print FILE "$profilecert_dir does not exist: skipping deleting cpath\n";
        return 0;
    }

    chdir "$profilecert_dir" or die "change dir failed: $!";
    opendir(DIR, ".");
    my @flist = readdir(DIR);
    # Delete any existing symbolic links
    foreach (grep {/^[\da-f]+\.r{0,1}\d+$/} @flist) {
        if(-l $_) {
            print FILE "file=$_\n";
            $certlink = readlink $_;
            if ($certlink eq  "$profile_dir/$fname") {
                print FILE "found=$certlink\n";
                unlink $_;
            }
        }
    }
    closedir DIR;
    my ($ret) = regen_cafile();
    if ($ret == 1) {
        return 1;
    }

    return 0;

}

sub check_file {
        my ($is_cert, $is_crl) = (0,0);
        my $fname = $_[0];
        open IN, $fname;
        while(<IN>) {
                if(/^-----BEGIN (.*)-----/) {
                        my $hdr = $1;
                        if($hdr =~ /^(X509 |TRUSTED |)CERTIFICATE$/) {
                                $is_cert = 1;
                                last if($is_crl);
                        } elsif($hdr eq "X509 CRL") {
                                $is_crl = 1;
                                last if($is_cert);
                        }
                }
        }
        close IN;
        return ($is_cert, $is_crl);
}


# Link a certificate to its subject name hash value, each hash is of
# the form <hash>.<n> where n is an integer. If the hash value already exists
# then we need to up the value of n, unless its a duplicate in which
# case we skip the link. We check for duplicates by comparing the
# certificate fingerprints

sub link_hash_cert {
                my $fname = $_[0];
                $fname =~ s/'/'\\''/g;
                my ($hash, $fprint) = `"$openssl" x509 -hash -fingerprint -noout -in "$fname"`;
                chomp $hash;
                chomp $fprint;
                $fprint =~ s/^.*=//;
                $fprint =~ tr/://d;
                my $suffix = 0;
                # Search for an unused hash filename
                while(exists $hashlist{"$hash.$suffix"}) {
                        # Hash matches: if fingerprint matches its a duplicate cert
                        if($hashlist{"$hash.$suffix"} eq $fprint) {
                                print FILE "WARNING: Skipping duplicate certificate $fname\n";
                                return;
                        }
                        $suffix++;
                }
                $hash .= ".$suffix";
                print "$fname => $hash\n";
                $symlink_exists=eval {symlink("",""); 1};
                if ($symlink_exists) {
                        symlink $fname, $hash;
                } else {
                        open IN,"<$fname" or die "can't open $fname for read";
                        open OUT,">$hash" or die "can't open $hash for write";
                        print OUT <IN>; # does the job for small text files
                        close OUT;
                        close IN;
                }
                $hashlist{$hash} = $fprint;
}

# Same as above except for a CRL. CRL links are of the form <hash>.r<n>

sub link_hash_crl {
                my $fname = $_[0];
                $fname =~ s/'/'\\''/g;
                my ($hash, $fprint) = `"$openssl" crl -hash -fingerprint -noout -in '$fname'`;
                chomp $hash;
                chomp $fprint;
                $fprint =~ s/^.*=//;
                $fprint =~ tr/://d;
                my $suffix = 0;
                # Search for an unused hash filename
                while(exists $hashlist{"$hash.r$suffix"}) {
                        # Hash matches: if fingerprint matches its a duplicate cert
                        if($hashlist{"$hash.r$suffix"} eq $fprint) {
                                print FILE "WARNING: Skipping duplicate CRL $fname\n";
                                return;
                        }
                        $suffix++;
                }
                $hash .= ".r$suffix";
                print "$fname => $hash\n";
                $symlink_exists=eval {symlink("",""); 1};
                if ($symlink_exists) {
                        symlink $fname, $hash;
                } else {
                        system ("cp", $fname, $hash);
                }
                $hashlist{$hash} = $fprint;
}

sub regen_cafile {
    chdir "$trustedStore_dir" or die "change dir failed: $!";
    my ($ret) = is_dir_empty($profile_dir);
    if ($ret == 1) {
        print FILE "The $profile_dir is empty.\n";
        if (-e "$trustedStore_dir/$profile_cafile") {
            print FILE "find $profile_cafile.\n";
            unlink "$trustedStore_dir/$profile_cafile";
            qx(touch "$trustedStore_dir/$profile_cafile");
        }
    } else {
        unlink "$trustedStore_dir/$profile_cafile";
        chdir "$profile_dir";
        opendir(DIR, ".");
        my @flist = readdir(DIR);
        closedir DIR;
        #FILE: foreach $fname (grep {/\.pem$/} @flist) {
        FILE: foreach $fname (@flist) {
                if ($fname eq "." || $fname eq "..") {
                    next;
                }

                # Check to see if certificates and/or CRLs present.
                my ($cert, $crl) = check_file($fname);
                if(!$cert && !$crl) {
                        print FILE "WARNING: $fname does not contain a certificate or CRL: skipping\n";
                        #system("${IMA_SVR_INSTALL_PATH}/bin/icu_gettext", "CWLNA8229", "ERROR");
                        next;
                }
                my ($regen_cafile_ret) = `"$openssl" x509 -in "$fname" -text >> "$trustedStore_dir/$profile_cafile"`;
                if ($regen_cafile_ret == 1) {
                    print FILE "cannot find $fname.\n";
                    return $regen_cafile_ret;
                }
        }
    }
    return 0;
}

sub is_dir_empty {
    my ($dir) = @_;

    opendir my $handle, $dir
        or die "Cannot open directory: '$dir': $!";

    while ( defined (my $dentry = readdir $handle) ) {
        return unless $dentry =~ /^[.][.]?\z/;
    }

    return 1;
}

