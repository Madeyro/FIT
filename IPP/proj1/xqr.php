<?php 

#XQR:xkopec44

/**
 * XML query
 *
 * Simple XML query script. Created as school project.
 *
 * PHP version 5.3.3
 *
 * @category   School project
 * @author     Maros Kopec, xkopec44@stud.fit.vutbr.cz
 * @since      File available since March 2016
 */

################################### Script ####################################
error_reporting(E_ERROR | E_PARSE);

$options = handle_arg($argc, $argv);

### Get input from file or stdin
if (isset($options["input"])) {
    $xml_in = file_getcheck($options["input"]);
}
else {
    $xml_in = file_get_contents("php://stdin");
}

### Create XML object based on input
try {
    $xml_in = new SimpleXMLElement($xml_in);
} catch(Exception $e) {
    handle_error("Input file could not be parsed as XML.\n", 4);
}

### Parse query
if (isset($options["query"])) {
    $query = handle_query($options["query"]);
}
elseif (isset($options["qf"])) {
    $qstring = file_getcheck($options["qf"]);
    $query = handle_query($qstring);
}

### Execute query
$xml_out = execute_query($xml_in, $query);
    if ($xml_out === FALSE) {
        $xml_out = "";
    }

// ROOT element
if (isset($options["root"])) {
    $xml_out = "<$options[root]>\n" . $xml_out . "\n</$options[root]>";
}

// if -n parameter is, script do not generate XML header
if ($options["noheader"]) {
    $header = "";
}
else {
    $header = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
}
$xml_out = $header . $xml_out;

### Create or write to XML file
if (isset($options["output"])) {
    file_putcheck($options["output"], $xml_out);
}
else {
    file_put_contents("php://stdout", $xml_out);
}

exit(0); // success



################################## Functions ##################################


 /** 
 *
 * Handle error reporting and exit script
 *
 * exit codes:
 * 1. wrong parameters format or wrong combination
 * 2. do not exist input file
 * 3. error during opening file, or writing to file
 * 4. wrong input file format
 * 10. - 99. specific for assignment -> 80 when syntax or semantic error in query 
 * 100. - 127. other
 * 
 * @param string    Error message
 * @param int       Error code
 *
 */
function handle_error($msg, $code) {
    file_put_contents("php://stderr", $msg);
    exit($code);
}


/** 
 *
 * Handle arguments and check if they are wrong
 * 
 * @param int       argc, number of arguments 
 * @param array     argv, array of arguments
 *
 * @return array    parsed arguments
 */
function handle_arg($argc, $argv) {
    $longopts  = array(
        "input:",
        "output:",
        "query:",
        "qf:",
        "root:",      
    );
    $options = getopt(null, $longopts);

    if ($argc < 2) {
        handle_error("No arguments. Try --help\n", 1);
    }
    elseif ($argc == 2 && $argv[1] === "--help") {
        print_help();
    }
    elseif (count($options) == 0) {
        handle_error("Wrong arguments.\n", 1);
    }
    else {
        foreach ($argv as $key => $value) {
            if ($value === "--help") {
                handle_error("Wrong combinations of arguments.\n", 1);
            }
        }
    }

    // Check for short argument "-n"
    foreach ($argv as $arg) {
        if (preg_match("/^-\w*n\w*/", $arg) === 1) {
            if (preg_match("/-n$/", $arg) === 1) {
                $options["noheader"] = TRUE;
                break;
            }
            else {
                handle_error("Wrong short argument.\n", 1);
            }    
        }
        else {
            $options["noheader"] = FALSE;
        }
    }

    // More then one occurs of argument
    foreach ($options as $opt) {
        if (count($opt) > 1) {
            handle_error("Repeated argument/s.\n", 1);
        }
    }

    if (array_key_exists("qf", $options)) {
        if (array_key_exists("query", $options)) {
            handle_error("Wrong combination of arguments. Can not be used --qf with --query\n", 1);
        }
        if ( !file_exists($options["qf"]) ) { // DO NOT exist
            handle_error("Do not exist query file \"$options[qf]\".\n", 80);        
        }
    }
    elseif (!array_key_exists("query", $options)) { // DO NOT exist
        handle_error("You must specify either --qf (query file) or --query='task'.\n", 80);
    }

    if (array_key_exists("input", $options)) {
        if ( !file_exists($options["input"]) ) { // DO NOT exist
            handle_error("Do not exist input file \"$options[input]\".\n", 2);
        }
    }
    
    return $options;
}


/** 
 *
 * Handle query and check for syntax or semantic errors
 * 
 * @param string    string containing query 
 * 
 * @return array    parsed query
 */
function handle_query($qstring) {
    
    preg_match("/(SELECT.*(LIMIT)?.*FROM.*(WHERE)?.*(NOT)?.*)/", $qstring, $match);
    if ( !isset($match[0])) { // is NOT set
        handle_error("Syntax or semantix error.\n", 80);
    }

    preg_match("/(?<=SELECT\s)\s*\K\w+/", $qstring, $match);
    if (isset($match[0])) {
        if ($match[0] === "FROM" || $match[0] === "LIMIT") {
            handle_error("Missing SELECT element.\n", 80);
        }
        $query["select"] = $match[0];
    }
    else {
        handle_error("Wrong syntax near SELECT.\n", 80);
    }

    // [+|-]? to match also sign near number
    if (preg_match("/LIMIT/", $qstring) === 1) {
        preg_match("/(?<=LIMIT\s)\s*\K\d+/", $qstring, $match);
        if (isset($match[0])) {
            $query["limit"] = $match[0];
        }
        else {
            handle_error("Syntax error near LIMIT.\n", 80);
        }    
    }

    preg_match("/(?<=FROM\s)\s*\K\w*[.]?\w+/", $qstring, $match);
    if (isset($match[0])) {
        $match = preg_split("/\./", $match[0]);
        // echo ">>>" . $match[0] . "<<< >>>" . $match[1] . "<<<\n";
        if ( !($match[0] === "")) { // is NOT empty string
            $query["from"]["element"] = $match[0];
        } 
        if (isset($match[1]))
            $query["from"]["attribute"] = $match[1];
    }
    else {
        $query["from"] = FALSE; // script will return empty file
    }

    if (preg_match("/WHERE/", $qstring) === 1) {
        preg_match("/(?<=WHERE\s)\s*\K(?:NOT\s+)*[\.]?\w+\s*\w*[\.]?\w+\s*(?:=|>|<|CONTAINS)?\s*(?:\"\w+\"|\+\d+|\-\d+|\d+){1}(?!\.)/", $qstring, $match);
        if (isset($match[0])) {
            preg_match_all("/\s+\KNOT(?=\s+)/", $qstring, $match);
            if (isset($match[0])) {
                if (count($match[0]) % 2 === 0) {
                    $query["where"]["not"] = FALSE; // double negation
                }
                else {
                    $query["where"]["not"] = TRUE; // contains NOT and condition will be negated
                }
            }
            else {
                $query["where"]["not"] = FALSE; // no NOT
            }
            preg_match("/\s+(?:WHERE|NOT){1}\s+\K[^NOT|\"]\w*[.]?\w+/", $qstring, $match);
            if (isset($match[0])) {
                $match = preg_split("/\./", $match[0]);
                if ( !($match[0] === "")) { // is NOT empty string
                    $query["where"]["element"] = $match[0];
                }
                // $query["where"]["element"] = $match[0];
                if (isset($match[1])) {
                    $query["where"]["attribute"] = $match[1];
                }
            }
            preg_match("/\w+\s+\KCONTAINS\s+(?=\"\w+\")/", $qstring, $match); // CONTAINS substring
            if (isset($match[0])) {
                $query["where"]["operator"] = "CONTAINS";
            }
            else {
                if ( preg_match("/CONTAINS/", $qstring) === 1 ) {
                    handle_error("Wrong syntax near CONTAINS.\n", 80);
                }
                preg_match("/\w+\s*\K(=|(>|<){1}\s*(?=\+\d+|\-\d+|\d+)){1}/", $qstring, $match); // >, <, =
                if (isset($match[0])) {
                    $match[0] = preg_replace("/\s+/", "", $match[0]);
                    $query["where"]["operator"] = $match[0];
                }
            }
            preg_match("/(?<=>|<|=|CONTAINS)\s*(?:\"[\w\s]+\"|\+\d+|\-\d+|\d+){1}/", $qstring, $match);
            if (isset($match[0])) {
                $match[0] = preg_replace("/(?<!\w)\s+(?!\w)/", "", $match[0]); // remove white spaces
                // $match[0] = preg_replace("/\"/", "", $match[0]); // remove quotes
                $query["where"]["literal"] = $match[0]; // array key literal represents literal or number
            }
            
            if (count($query["where"]) < 4) {
                handle_error("Wrong syntax near WHERE clause. Something is missing.\n", 80);
            }
        }
        else {
            handle_error("Wrong syntax near WHERE clause.\n", 80);
        }
    }

    return $query;
}


/** 
 *
 * Select needed parts of XML based on query
 * 
 * @param SimpleXMLElement      xml file
 * @param array                 parsed query
 *
 * @return mixed     XML as string or may return FALSE for unsuccessful search
 */
function execute_query($xml, $query) {
    
    // Iterative applying filters, to the point we have wanted result
    
    // Filter FROM where we select
    if ($query["from"] === FALSE) {
        return FALSE;
    }
    if (isset($query["from"]["element"])) {
        // echo $query["from"]["element"] . "\n";
        if ($query["from"]["element"] === "ROOT") {
            $xpath_from = "//*";
        }
        else {
            $xpath_from = "//" . $query["from"]["element"];
        }
        if (isset($query["from"]["attribute"])) {
            $xpath_from = $xpath_from . "[@" . $query["from"]["attribute"] . "]";
        }
    }
    else {
        $xpath_from = "//*" . "[@" . $query["from"]["attribute"] . "]";
    }

    $xml = $xml->xpath($xpath_from);

    if ($xml === FALSE) {
        return $xml;
    }
    else {
        $xml = $xml[0];
    }
    
    
    // Filter what element we SELECT
    if (isset($xml)) {
        $xpath_select = ".//" .$query["select"];
        $xml = $xml->xpath($xpath_select);
        if ($xml === FALSE) {
            return $xml;
        }
    }
    else {
        return FALSE;
    }
    
    // Filter conditions (WHERE)
    if (isset($query["where"])) {
        if ($query["where"]["operator"] === "CONTAINS") {
                if (isset($query["where"]["attribute"])) {
                    $xpath_where = $xpath_where . "@{$query[where][attribute]}";
                } else {
                    $xpath_err4 = "//{$query[select]}//{$query[where][element]}/*"; // Check validity of XML
                    /* SELECT element1 FROM element2 WHERE element3 ... 
                    *  When element3 contains with text also another elements (children),
                    *  it is considered not valid XML file for script and return error code 4 */
                    foreach($xml as $data) {
                        $xml_temp = $data->xpath($xpath_err4);
                    }
                    if ( !empty($xml_temp)) { // NOT empty
                        handle_error("Error, not valid XML file.\n", 4);
                    }
                    $xpath_where = $xpath_where . ".";
                }
                // wrap xpath contains function
                $xpath_where = "contains(" . $xpath_where . ", {$query[where][literal]})";
        }
        else {
            if (isset($query["where"]["attribute"])) {
                if (isset($query["where"]["element"])) {
                    $xpath_where = $xpath_where . $query["where"]["element"];
                }
                $xpath_where = $xpath_where . $query["where"]["attribute"];
            }
            else {
                $xpath_where = $xpath_where . $query["where"]["element"];
            }
            // add operator and literal/number
            $xpath_where = $xpath_where . $query["where"]["operator"] . $query["where"]["literal"];
        }
        
        if ($query["where"]["not"] === TRUE) {
            $xpath_where = "[not(" . $xpath_where . ")]";
        } else {
            $xpath_where = "[" . $xpath_where . "]";
        }
        
        $xpath_where = "//" . $query["select"] . $xpath_where;
        foreach($xml as $data) {
            $xml = $data->xpath($xpath_where);
        }
    }
    
    // Filter how much results we want
    if (isset($query["limit"])) {
        $xml = array_slice($xml, 0, $query["limit"]);
    }
        
    $res = ""; // so we can append elements as string
    if ( !empty($xml) ) { // NOT empty
        foreach ($xml as $element) {
            $res = $res . $element->asXML();
        }
    }
    
    return $res;

}


/** 
 *
 * Check if file is readable, and get contents
 * 
 * @param filename       checked filename 
 *
 * @return string        string containing file
 */
function file_getcheck($file) {
    if (is_readable(realpath($file))) {
        $fstring = file_get_contents($file);
    }
    else {
        handle_error("Error during opening file: $file.\n", 3);
    }
    if ($fstring === FALSE) {
        handle_error("Error during reading file: $file.\n", 3);
    }
    else {
        return $fstring;
    }
}


/** 
 *
 * Check if file is writable, and put contents
 * 
 * @param filename      checked filename 
 * @param string        content that will be written
 *
 * @return void
 */
function file_putcheck($file, $content) {
    if (file_exists(realpath($file))) {
        if (is_writable(realpath($file))) {
            file_put_contents(realpath($file), $content);
        }
        else {
            handle_error("Error during writing to a file: $file.\n", 3);
        }
    }
    else {
        $handle = fopen($file, "w");
        fwrite($handle, $content);
        fclose($handle);
    }
    
    return; // void
}


/** 
 *
 * Print help
 *
 */
function print_help() {
    echo (
        "###########################################################################\n".
        "\nUsage:".
        "xqr.php [options]\n\n".
        "Options:\n".
        "\t--input=<file>    Load XML file that will be processed\n".
        "\t--output=<file>   Choose file, where should be result saved\n".
        "\t--qf=<file>       Task saved in external file,\n".
        "\t                  can't be combinated with --query=<\"task>\"\n".
        "\t--query=\"<task>\"  Task that will be executed on xml file\n".
        "\t--root=element    Name of xml root\n".
        "\t-n                Do not generate xml head on script output\n".
        "\t--help            Print this help\n".
        "\n".
        "Returns:\n".
        "\t1. wrong parameters format or wrong combination\n".
        "\t2. do not exist input file\n".
        "\t3. error during opening file, or writing to file\n".
        "\t4. wrong input file format\n".
        "\t80. when syntax or semantic error in query\n".
        "\n###########################################################################\n"
    );
    exit(0);
}
 
?>