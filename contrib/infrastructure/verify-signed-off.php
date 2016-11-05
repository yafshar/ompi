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
# Fill in your configuration values in this routine
##############################################################################
##############################################################################

if (!is_file("verify-signed-off-config.php")) {
    my_die("Cannot find verify-signed-off.php's config file.");
}
require_once "verify-signed-off-config.php";

##############################################################################
##############################################################################
# You should not need to change below this line
##############################################################################
##############################################################################

# Functions

function my_die($msg)
{
    # Die with a non-200 error code
    http_response_code(400);

    die($msg);
}

function debug($config, $str)
{
    if (isset($config["debug"]) && $config["debug"]) {
        print($str);
    }
}

function check_for_allowed_sources($config)
{
    global $config;

    if (isset($config["allowed_sources"]) &&
        count($config["allowed_sources"] > 0)) {
	if (isset($_SERVER["HTTP_X_REAL_IP"])) {
            $source = ip2long($_SERVER["HTTP_X_REAL_IP"]);
        } else if (isset($_SERVER["REMOTE_ADDR"])) {
            $source = ip2long($_SERVER["REMOTE_ADDR"]);
        } else {
            # This will not match anything
            $source = 0;
        }

        $happy = 0;
        foreach ($config["allowed_sources"] as $cidr) {
            $parts = explode('/', $cidr);
            $value = ip2long($parts[0]);
            $mask = (pow(2, 33) - 1) - (pow(2, $parts[1] + 1) - 1);

            if (($value & $mask) == ($source & $mask)) {
                $happy = 1;
            }
        }
        if (!$happy) {
            my_die("Discarding request from disallowed IP address (" .
            $_SERVER["HTTP_X_REAL_IP"] . ")\n");
        }
    }
}

function check_for_non_empty_payload()
{
    # Ensure we got a non-empty payload
    if (!isset($_POST["payload"])) {
        my_die("Received POST request with empty payload\n");
    }
}

function parse_json()
{
    # Parse the JSON
    $json = json_decode($_POST["payload"]);
    if (json_last_error() != JSON_ERROR_NONE) {
        my_die("Got invalid JSON\n");
    }

    return $json;
}

function fill_opts_from_json($json)
{
    # If there is no "before" property, then it's just a github ping
    # and we can ignore it
    if (!isset($json->{"before"}) ||
        !isset($json->["action"]) ||
        ($json->["action"] != "synchronize" &&
         $json->["action"] != "opened")) {
       print "Hello, Github ping!  I'm here!\n";
       exit(0);
    }

    $url = $json->{"repository"}->{"url"};
    $opts["link"] =
        "$url/compare/" . $json->{"before"} . "..." . $json->{"after"};

    $opts["repo"] = $json->{"repository"}->{"full_name"};
    $opts["url"] = $json->{"repository"}->{"url"};

    $opts["before"] = $json->{"before"};
    $opts["after"] = $json->{"after"};
    $opts["refname"] = $json->{"ref"};

    return $opts;
}

function fill_opts_from_keys($config, $opts, $arr)
{
    # Deep copy the keys/values into the already-existing $opts
    # array
    foreach ($arr as $k => $v) {
        $opts[$k] = $v;
    }

    # Was the URL set?
    if (!isset($opts["uri"]) && isset($config["url"])) {
        $opts["uri"] = $config["url"];
    }

    return $opts;
}

function get_value($config, $opts, $key)
{
    if (isset($opts[$key])) {
        return $opts[$key];
    } else if (isset($config[$key])) {
        return $config[$key];
    }

    return null;
}


function process($json, $config, $opts, $key, $value)
{
    $opts = fill_opts_from_keys($config, $opts, $value);

    # This webhook will have only delivered the *new* commits on this
    # PR.  We need to examine *all* the commits on this PR -- so we
    # can discard the commits that were delivered in this webhook
    # payload.  Instead, do a fetch to get all the commits on this PR
    # (note: putting the Authorization header in this request just in
    # case this is a private repo).

    $commits_url = $json->["pull_request"]["commits_url"];

    # JMS ORIGINAL RUBY
  commits_url = push['pull_request']['commits_url']
  commits = HTTParty.get(commits_url,
      :headers => {
        'Content-Type'  => 'application/json',
        'User-Agent'    => user_agent,
        'Authorization' => "token #{auth_token}" }
    )

  # Sanity check: If we've got no commits, do nothing.
  if commits.length == 0 then
    return [200, 'Somehow there are no commits on this PR... Weird...']
  end


    # Happiness!  Exit.
    exit(1);
}

##############################################################################
# Main

# Verify that this is a POST
if (!isset($_POST) || count($_POST) == 0) {
    print("Use " . $_SERVER["REQUEST_URI"] . " as a WebHook URL in your Github repository settings.\n");
    exit(1);
}

# Read the config
$config = fill_config();

# Sanity checks
check_for_allowed_sources($config);
check_for_non_empty_payload();

$json = parse_json();
$opts = fill_opts_from_json($json);

# Loop over all the repos in the config; see if this incoming request
# is from one we recognize
$repo = $opts["repo"] = $json->{"repository"}->{"full_name"};

# The keys of $config["github"] are repo names (e.g.,
# "open-mpi/ompi").  The keys of $config["github"][$repo_name]
# are all the config values for that repo.
foreach ($config["github"] as $key => $value) {
    debug($config, "Checking github id ($repo) against: $key<br />\n");
    if ($repo == $key) {
        debug($config, "Found match!\n");

        process($json, $config, $opts, $key, $value);

        # process() will not return, but be paranoid anyway
        exit(0);
    }
}

# If we get here, it means we didn't find a repo match
my_die("no matching repository found for $repo");
