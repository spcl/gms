#! /bin/python3

## imports
import sys, os, getopt
import re, json
import shutil
import copy
from collections import defaultdict, namedtuple
from datetime import datetime
import pandas as pd 

#########################################################################################################################
## scriptmaker classes
class ConfigException(Exception):
    """class for errors in config file"""
    pass

class FileOptions:
    """Represent options of a bash-file."""
    def __init__(self):
        self.reset()

    def reset(self):
        self.Shebang = False
        self.ParamInfo = "@##"
        self.ParamValue = "@@#"
        self.ProgramOutput = "@@@"

class ScriptMaker:
    """Class to create a bash-script.
    
    Can be used with the 'with .. as .. ' syntax."""
    def __init__(self, name=None):
        if name==None:
            self._name = 'runscript.sh'
        else:
            self._name = name
        self._entered = False
        self.FileOptions = FileOptions()

    def _checkfile(self):
        if not self._entered:
            raise IOError("No file opened")

    def __enter__(self):
        self._file = open(self._name, 'w')
        self._entered = True
        self.FileOptions.reset()
        return self

    def __exit__(self, type, value, tb):
        self._file.close()
        self._entered = False
        self.FileOptions.reset()

    def writeLine(self, arg):
        self._checkfile()
        self._file.write(arg + "\n")
    
    def writeDirective(self, directive, value):
        pass

    def writeTask(self, line, options=None):
        self.writeLine(line)

    def writeJob(self, line, options=None):
        self.writeLine(line)
    
    def echoParamInfo(self, arg):
        line = "echo \"" + self.FileOptions.ParamInfo + ' ' + arg + "\""
        self.writeLine(line)

    def echoParamValue(self, arg):
        line = "echo \"" + self.FileOptions.ParamValue + ' ' + arg + "\""
        self.writeLine(line)

    def shebang(self, path="/bin/bash"):
        if self.FileOptions.Shebang:
            return
        self.writeLine("#! {}".format(path))
        self.FileOptions.Shebang = True

class SBatchScriptMaker(ScriptMaker):
    """Class to write a sbatch script for slurm."""
    def __init__(self, name=None, options=None):
        super(SBatchScriptMaker, self).__init__(name=name)

    def shebang(self, path="/bin/bash"):
        super(SBatchScriptMaker, self).shebang(path + ' -l')

    def writeDirective(self, directive, value):
        self.writeLine("#SBATCH --{}={}".format(directive, value))

    def writeTask(self, line, options=''):
        try:
            parsed = '-c {}'.format(getattr(options, "threads"))
        except:
            parsed = '-c 1'
        self.writeLine("srun {} {} ; p".format(parsed, line))
    
    def writeJob(self, line, options=None):
        self.writeLine("sbatch "+line)

class BsubScriptMaker(ScriptMaker):
    """Class to write a bsub script for lsf."""
    def __init__(self, name=None):
        super(BsubScriptMaker, self).__init__(name=name)
    
    def writeDirective(self, directive, value):
        self.writeLine("#BSUB -{} {}".format(directive, value))
    
    def writeJob(self, line, options=None):
        self.writeLine("bsub < "+line)

## end scriptmaker classes
#########################################################################################################################
## parse options
def stripComment(string, comment_prefix='//'):
    def replacer(match):
        s = match.group(0)
        if s.startswith(comment_prefix):
            return ''
        return s
    pattern = re.compile(comment_prefix + r'.*|"(?:\\.|[^\\"])*"|\'(?:\\.|[^\\\'])*', 
                re.MULTILINE)
    return re.sub(pattern, replacer, string)
            
def paramType(param_location):
    """Return type of parameter: none/named/positional."""
    if param_location == None or str(param_location) == 'none':
        return 'none'
    try:
        if int(param_location) > 0:
            return 'positional'
    except:
        pass
    return 'named'

def listRange(start, stop, adder):
    """Create a linear range from start up to (including) stop in adder steps."""
    res = list()
    while start <= stop:
        res.append(start)
        start = start + adder
    return res

def parseToValueList(item):
    """Parse a range item into a list of values."""
    res = list()
    if not isinstance(item, str):
        item = str(item)
    try:
        splits = item.split(':')
        if '.' in item:
            # we have floats
            if len(splits) == 3:
                start = float(splits[0])
                stop = float(splits[1])
                adder = float(splits[2])
            elif len(splits) == 2:
                start = float(splits[0])
                stop = float(splits[1])
                adder = 1.
            else:
                start = float(splits[0])
                stop = start + 0.1 # ensure at least one value (num prec)
                adder = 1.
        else:
            # we have int
            if len(splits) == 3:
                start = int(splits[0])
                stop = int(splits[1])
                adder = int(splits[2])
            elif len(splits) == 2:
                start = int(splits[0])
                stop = int(splits[1])
                adder = 1
            else:
                start = int(splits[0])
                stop = start
                adder = 1
        tmp = listRange(start, stop, adder)
    except:
        tmp = [item]
    res.extend( [str(e) for e in tmp] )
    return res

def parseParameterValues(values):
    if not isinstance(values, list):
        tmp = parseToValueList(values)
        return tmp
    
    result = []
    if isinstance(values[0], list):
        for value in values:
            list2d = []
            for item in value:
                list2d.append( parseToValueList(item))
            len1d = len(list2d[0])
            for lst in list2d:
                if len(lst) != len1d:
                    print("len1d: ", len1d)
                    print("lst: ", lst)
                    print("list2d[0]:", list2d[0])
                    raise ConfigException("Different number of values in parameter-tandem")
            for i in range(len1d):
                tmp = [ list1d[i] for list1d in list2d ]
                result.append(tmp)
    else:   
        for value in values:
            tmp = parseToValueList(value)
            result.extend(tmp)
    return result

def checkParameterValues(key, values):
    keylen = len(key.split())
    if keylen == 1:
        if not isinstance(values, list):
            return
        for elem in values:
            if isinstance(elem, list):
                raise ConfigException("values of parameter '{}' cannot contain nested list".format(key))
    else:
        if not isinstance(values, list):
            raise ConfigException("values of parameter '{}' must be nested list".format(key))
        for elem in values:
            if not (isinstance(elem, list) and len(elem) == keylen):
                raise ConfigException("values of parameter '{}' must be nested lists of length {}".format(key, keylen))

def checkParameterTypes(param_locations, dtype=None, alg=''):
    if dtype == None and len(param_locations) > 0:
        loc1 = list(param_locations.values())[0]
        if isinstance(loc1, list) and len(loc1) > 0:
            dtype = paramType(loc1[0])
        elif isinstance(loc1, list):
            dtype = 'none'
        else:
            dtype = paramType(loc1)
    elif dtype == None:
        dtype = 'none'

    if len(param_locations) > 0:
        locValues = list(param_locations.values())
        for locValue in locValues:
            if isinstance(locValue, list):
                for lv in locValue:
                    if dtype != paramType(lv):
                        raise ConfigException("parameters_locations of algorithm '{}' ".format(alg)+
                            "do not have consistent type")
            else:
                if dtype != paramType(locValue):
                    raise ConfigException("parameters_locations of algorithm '{}' ".format(alg)+
                        "do not have consistent type")
    else:
        if dtype != 'none':
            raise ConfigException("parameters_location of algorithm '{}' ".format(alg)+
                "do not have consistent type")
    return dtype

def parseOptions(json_options):
    """Parse the raw son options.
    
    Parses the parameter values into ranges and adds missing information
    that can be inferred from other values.
    
    Returns parsed options as dictionary"""
    parsed = dict(json_options)
    if "algorithms" in json_options:
        algorithms = copy.deepcopy(json_options["algorithms"])
    else:
        algorithms = list()

    pruned_alg = list()
    for alg in algorithms:
        if "ignore" in alg and alg["ignore"] == "true":
            continue
        else:
            pruned_alg.append(alg)
    algorithms = pruned_alg

    for alg in algorithms:
        # check for positional/named arguments
        if "parameters_type" in alg:
            checkParameterTypes(alg["parameters_location"], alg["parameters_type"], extractName(alg["executable"]))
        else:
            pt = checkParameterTypes(alg["parameters_location"], alg=extractName(alg["executable"]))
            alg["parameters_type"] = pt

        option = dict()
        for k in alg["parameters_location"]:
            if k in alg["parameters_values"]:
                option[k] = parseParameterValues( alg["parameters_values"][k])
            elif "common_parameter_values" in json_options and k in json_options["common_parameter_values"]:
                option[k] = parseParameterValues( json_options["common_parameter_values"][k])
            else:
                raise ConfigException("Key '{}' not found in \"parameters_values\" or \"common_parameters_values\"".format(k))
        alg["parameters_values"] = option

    parsed["algorithms"] = algorithms
    return parsed

## end parse options
#########################################################################################################################
## cartesian product / parameter space

def spanCartesionProduct(algorithm):
    """Span the parameter space of a single algorithm.
    
    Spans the parameters space as the cartesian product of the options.
    
    Keyword arguments:
    algorithm -- parsed json configuration of a single algorithm.
    
    Returns: pandas DataFrame."""
    dfs = []
    for colsName in algorithm["parameters_location"]:
        df = pd.DataFrame(
                data=algorithm["parameters_values"][colsName], columns=colsName.split()
                )
        df['temp_key_'] = 0
        dfs.append(df)

    cp =  pd.DataFrame(data=[algorithm["executable"]], columns=['executable'] )
    cp['temp_key_'] = 0

    for i in range(len(dfs)):
        cp = pd.merge(cp, dfs[i], how='outer', on='temp_key_')

    
    cp.drop(columns=['temp_key_'], inplace=True)
    namePosPairs = []
    for k in algorithm["parameters_location"]:
        val = algorithm["parameters_location"][k]
        if isinstance(val, list):
            keys = k.split()
            for i in range(len(val)):
                namePosPairs.append( (keys[i], val[i]) )
        else:
            namePosPairs.append( (k, val) )
    posDict = { tp[1]: tp[0] for tp in namePosPairs }
    if algorithm["parameters_type"] == "positional":
        ckeys = [ int(p) for p in posDict.keys()]
        ckeys.sort()
        ckeys = [ str(p) for p in ckeys ]
    else:
        ckeys = list(posDict.keys())
        ckeys.sort()
    pcols = [ posDict[k] for k in ckeys]
    return cp[ ["executable"] + pcols]

def spanParameterSpace(options):
    """Span the parameter space as the cartesian product of the options per algorithm.

    Keyword arguments:
    optios -- parsed json configuration
    
    Returns: pandas DataFrame."""
    df = pd.DataFrame()
    for alg in options["algorithms"]:
        algdf = spanCartesionProduct(alg)
        df = df.append(algdf, ignore_index=True, sort=False)
    return df
## end cartesian product / parameter space
#########################################################################################################################
## create single scripts
def extractName(arg):
    """Try to extract a basename from the provided argument."""
    if not isinstance(arg, str):
        arg = str(arg)
    try:
        name = os.path.basename(arg)
    except:
        name = arg

    fileExtensions = ['el', 'txt', 'dat', 'csv']
    for ext in fileExtensions:
        name = re.sub(r'\.'+'{}$'.format(ext), '', name)
    name = re.sub(r'\.', '_', name)
    return name

def generateNameSuffix():
    """Create time stamp string."""
    return datetime.now().strftime("%Y%m%d_%H%M%S")

def scriptAlgorithmUnit(param_space, options, maker):
    """Write script with all parameter configurations for a single algorithm.

    Keyword arguments:
    param_space -- pandas DataFrame with all relevant parameter configurations.
    options -- The parsed json configuration.
    maker -- Script writer. Should inherit from ScriptMaker/adhere to its interface"""
    # drop irrelevant columns
    #param_space = param_space.dropna(axis='columns', how='all')

    namePosPairs = []
    for k in options["parameters_location"]:
        val = options["parameters_location"][k]
        if isinstance(val, list):
            keys = k.split()
            for i in range(len(val)):
                namePosPairs.append( (keys[i], val[i]) )
        else:
            namePosPairs.append( (k, val) )
    posDict = { tp[1]: tp[0] for tp in namePosPairs }
    if options["parameters_type"] == "positional":
        ckeys = [ int(p) for p in posDict.keys()]
        ckeys.sort()
        ckeys = [ str(p) for p in ckeys ]
    else:
        ckeys = list(posDict.keys())
        ckeys.sort()
    pcols = [ posDict[k] for k in ckeys]

    param_space = param_space[ ["executable"]+pcols ]

    # write info: which columns/parameter
    line = ''
    for col in param_space.columns:
        line = line + ' ' + str(col)
    maker.echoParamInfo(line)

    if options["parameters_type"] == 'none':
        algName = extractName(options["executable"])
        for row in param_space.itertuples(index=False):
            maker.echoParamValue(algName)
            maker.writeTask(getattr(row, "executable"), row)

    elif options["parameters_type"] == 'positional':
        for row in param_space.itertuples(index=False):
            line = ""
            for elem in row:
                line = line + ' ' + str(elem)
            maker.echoParamValue(line)
            maker.writeTask(line, row)

    elif options["parameters_type"] == 'named':    
        prefix = dict()
        for c in param_space.columns:
            if c in options["parameters_location"]:
                prefix[c] = options["parameters_location"][c]
            else:
                inserted = False
                for k in options["parameters_location"]:
                    keys = k.split()
                    for i in range(len(keys)):
                        if c == keys[i]:
                            prefix[c] = options["parameters_location"][k][i]
                            inserted = True
                            break
                    if inserted:
                        break
                if not inserted:
                    prefix[c] = ''
                
        columns = list(param_space.columns[1:])
        for row in param_space.itertuples(index=False):
            paramValues = getattr(row, "executable")
            line = getattr(row, "executable")
            for col in columns:
                pval = str(getattr(row, col))
                pf = str(prefix[col])
                paramValues = paramValues + ' ' + pval
                line = line + ' ' + pf + ' ' + pval
            maker.echoParamValue(paramValues)
            maker.writeTask(line, row)

    else:
        raise ConfigException("Invalid parameters_type for algorithm {}".format(options["executable"]))

def scriptUnit(param_space, options, maker):
    """Write to script for all configurations in param_space.
    
    Keyword arguments:
    param_space -- pandas DataFrame with all relevant parameter configurations.
    options -- parsed json options.
    maker -- script write class (should inherit from ScriptMaker/adhere to its interface)"""
    for alg in options["algorithms"]:
        alg_params = param_space[ param_space["executable"] == alg["executable"]]
        if alg_params["executable"].any():
            scriptAlgorithmUnit(alg_params, alg, maker)

def createSingleScripts(parameter_space, options, separator=[], name_prefix='', name_suffix='', outputDir='.', errorDir='.'):
    """Create single scripts calling the executable with the correct parameters.
    
    The single scripts call the algorithms with all configuration in parameters_space.
    The calls get split into different scripts according to the separator argument.
    
    Keyword arguments:
    parameter_space -- pandas DataFrame containing all possible/relevant configurations of the parameters
    options -- the parsed json configuration
    separator -- list with parameter names. Defines the parameters on which the parameter space is split, eg:
    <graphs>: script 1 treats graph1, script 2 treats graph 2 and so on.
    name_prefix -- add custom prefix to the script names
    name_suffix -- add custom suffix to the script names
    
    Return: The file names of the created scripts."""
    # perform checks on input arguments
    if separator == None:
        separator = []
    if isinstance(separator, str):
        separator = list(separator)
    if not isinstance(separator, list):
        raise ValueError("separator must be string or list of strings")
    for item in separator:
        if isinstance(item, list):
            raise ValueError("separator cannot be nested list")
    
    filenames = []
    # base case: no separator, put everything into the same script
    if len(separator) == 0:
        filename = 'run{}_{}.sh'.format(name_prefix, name_suffix)
        if options["batch_system"] == "slurm":
            with SBatchScriptMaker(filename) as maker:
                maker.shebang()
                try:
                    ncpus = parameter_space["threads"].astype(int).max()
                except:
                    ncpus = 1
                # this is default action on cscs
                #maker.writeDirective("job-name", "\"{}\"".format( filename[-3]))
                for kdirective in options["slurm"]["sbatch"]:
                    maker.writeDirective( kdirective, options["slurm"]["sbatch"][kdirective])
                
                # restrict to only use open MP (no mpi support so far)
                maker.writeDirective("nodes", 1)
                maker.writeDirective("ntasks", 1)
                maker.writeDirective("cpus-per-task", ncpus)
                maker.writeDirective("error", "{}/{}-%j.err".format(errorDir, filename[0:-3]))
                maker.writeDirective("output", "{}/{}-%j.out".format(outputDir, filename[0:-3]))
                maker.writeLine("export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK")

                maker.writeLine("function p() {")
                maker.writeLine("    rt=$?;")
                maker.writeLine(r"    if [[ ${rt} -ne 0 ]]; then")
                maker.writeLine("        sleep 2")
                maker.writeLine("    fi")
                maker.writeLine(r"    return ${rt}")
                maker.writeLine("}")

                scriptUnit(parameter_space, options, maker)
            
        elif options["batch_system"] == "lsf":
            try:
                ncpus = parameter_space["threads"].astype(int).max()
            except:
                ncpus = 1
            with BsubScriptMaker(filename) as maker:
                maker.shebang()
                for kdirective in options["lsf"]["bsub"]:
                    maker.writeDirective(kdirective, options["lsf"]["bsub"][kdirective])
                maker.writeDirective("n", ncpus)
                maker.writeDirective("e", "{}/{}-%J.err".format(errorDir, filename[0:-3]))
                maker.writeDirective("o", "{}/{}-%J.out".format(outputDir, filename[0:-3]))
                maker.writeDirective("J", filename[0:-3])
                
                scriptUnit(parameter_space, options, maker)
        elif options["batch_system"] == "local":
            try:
                ncpus = parameter_space["threads"].astype(int).max()
            except:
                ncpus = 1
            with ScriptMaker(filename) as maker:
                maker.shebang()
                maker.writeLine("{")
                scriptUnit(parameter_space, options, maker)
                outfile = "{}/{}.out".format(outputDir, filename[0:-3])
                errfile = "{}/{}.err".format(errorDir, filename[0:-3])
                maker.writeLine(r'}' + " 1>{} 2>{}".format(outfile, errfile))

        else:
            raise ConfigException("Only slurm/lsf/local supported as batch_system at the moment.")
        filenames.append( filename )
        return filenames
        
    
    # else perform recursion on separators
    currentSep = separator[0]
    newseparator = separator[1:]
    if not currentSep in parameter_space.columns:
        raise ValueError("invalid separator: '{}'".format(currentSep))
    for sepItem in parameter_space[ currentSep ].unique():
        pruned_param_space = parameter_space[ parameter_space[currentSep] == sepItem ]
        sname = extractName(sepItem)
        new_prefix = name_prefix + '_' + sname
        fileList = createSingleScripts(pruned_param_space, options, newseparator, new_prefix, name_suffix,
                    outputDir=outputDir, errorDir=errorDir)
        filenames.extend( fileList )
    return filenames  

## end create single scripts
#########################################################################################################################
## create masterscript

def createMasterScript(filenames, options, outputDir, errorDir):
    """Create top level script that calls all other scripts.
    
    Keyword arguments:
    filenames -- list with the names of the other scripts
    options -- the parsed json-configuration
    outputDir -- directory where the other scripts place their output
    
    Return value: name of masterscript"""
    with ScriptMaker("masterscript.sh") as maker:
        maker.shebang()
        maker.writeLine('if [ ! -d {} ]; then'.format(outputDir))
        maker.writeLine('    mkdir {}'.format(outputDir))
        maker.writeLine('fi')
        maker.writeLine('if [ ! -d {} ]; then'.format(errorDir))
        maker.writeLine('    mkdir {}'.format(errorDir))
        maker.writeLine('fi')
        if options["batch_system"] == "slurm":
            
            for filename in filenames:
                maker.writeJob("sbatch node/{}".format(filename))
        elif options["batch_system"] == "lsf":
            for filename in filenames:
                maker.writeLine("bsub < node/{}".format(filename))
        elif options["batch_system"] == "local":
            for filename in filenames:
                maker.writeLine("node/{}".format(filename))
        else:
            raise ConfigException("batch_system only supports slurm, lsf, local.")

    return "masterscript.sh"

## end create masterscript
#########################################################################################################################
## main program
def help():
    print("usage: {} [options]".format( sys.argv[0]))
    print("  -h, --help            print this message")
    print("  -i [x], --input=x     path to config file (default: ./run_config.json)")
    print("  -n [x], --name_suffix=[x] name suffix to be used for script-name (default: \"script\")")
    print("  -s [x], --separator=[x]   List of separators to split scripts on (default: \"graphs\")")
    print("                            If more than one separator is required, put them in")
    print("                            quotation marks. separators must be valid algorithm")
    print("                            parameters, or \"algorithms\" itself")
    print("  -o [x], --ouptut=[x]  output directory (default: ./rs_[timestamp])")

def main():
    """Main function.
    
    Read in config file, parse it and create single scripts as
    well as masterscript."""

    # set default parameters
    config_file_path = "run_config.json"
    name_suffix = 'script'
    separator = ["graphs"]
    output_suffix = generateNameSuffix() #timestamp
    outputDir = 'rs_' + output_suffix

    # parse command line arguments
    try:
        opts, _ = getopt.getopt(sys.argv[1:], 
            "hi:n:s:o:", ["help", "input=", "name_suffix=", "separator=", "output="])
    except getopt.GetoptError:
        help()
        sys.exit(2)

    for opt, arg in opts:
        if opt in {'-h', '--help'}:
            help()
            sys.exit()
        elif opt in {'-i', '--input'}:
            config_file_path = arg
        elif opt in {'-n', '--name_suffix'}:
            name_suffix = arg
        elif opt in {'-s', '--separator'}:
            separator = arg.split()
        elif opt in {'-o', '--output'}:
            outputDir = arg

    # create directories
    if os.path.isfile(outputDir) or not os.path.exists(outputDir):
        os.makedirs(outputDir)
    if os.path.isfile(outputDir+"/node") or not os.path.exists(outputDir+"/node"):
        os.makedirs(outputDir+"/node")

    # create per-node-scripts & master script
    with open(config_file_path) as f:
        configFile = f.read()
    configFile = stripComment(configFile)
    options = json.loads(configFile)
    parsedOptions = parseOptions(options)
    parameter_space = spanParameterSpace(parsedOptions)

    filenames = createSingleScripts(parameter_space, parsedOptions, separator, name_suffix=name_suffix,
                    outputDir="output", errorDir="error")
    masterfilename = createMasterScript(filenames, parsedOptions, 
                    outputDir="output", errorDir="error")
    
    # move scripts into correct directories
    [ os.rename(fn, "{}/node/{}".format(outputDir, fn)) for fn in filenames ]
    os.rename( masterfilename, "{}/{}".format(outputDir, masterfilename))
    # copy config file
    shutil.copy2(config_file_path, outputDir)

## end main program
#########################################################################################################################
        
if __name__ == "__main__":
    main()
