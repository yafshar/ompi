<?php
#
# Copyright (c) 2016 Jeffrey M. Squyres.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

##############################################################################
##############################################################################
# This file must contain the function fill_config().
##############################################################################
##############################################################################

function fill_config()
{
    # verify-signed-off.php will ignore any incoming POST that does
    # not come from these network blocks.  As of 9 Aug 2016, according
    # to
    # https://help.github.com/articles/what-ip-addresses-does-github-use-that-i-should-whitelist/:
    $config["allowed_sources"] = array("192.30.252.0/22");

    # JMS Fill me in
    $config["auth_token"] = "JMS to fill in";

    # These are global values that apply to all repos listed below,
    # unless each repo overrides them.
    $config["from"] = 'gitdub@example.com';
    $config["to"] = "someone-who-cares@example.com";
    $config["subject"] = "Git: ";
    $config["protocol"] = "https";

    # A simple repo
    $config["github"]["jsquyres/gitdub-php"]["to"] =
        "gitdub-php-commits@example.com";

    # You almost certainly want "debug" set to 0 (or unset)
    $config["debug"] = 0;

    # Return the $config variable
    return $config;
}
