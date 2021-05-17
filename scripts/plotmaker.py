import os, re, getopt, sys
import numpy as np 
import pandas as pd
from matplotlib import pyplot
from pathlib import Path

#####################################################################################
## small utils

from matplotlib import rcParams
rcParams.update({'figure.autolayout': True})

def shortenGraphName(arg):
    if not isinstance(arg, str):
        return arg
    name = basename(arg)
    return re.sub("___symmetrized-sorted", "", name)

def basename(arg):
    """Try to extract a basename from the provided argument."""
    if not isinstance(arg, str):
        return arg
    try:
        name = os.path.basename(arg)
    except:
        name = arg

    fileExtensions = ['el', 'txt', 'dat', 'csv', 'exe', 'out']
    for ext in fileExtensions:
        name = re.sub(r'\.'+'{}$'.format(ext), '', name)
    name = re.sub(r'\.', '_', name)
    return name

def readData(path, extractBasenames=True, shortenGraphNames=True):
    data = pd.read_csv(path, sep=" ", header=None)
    if extractBasenames:
        data = data.applymap(basename)
    if shortenGraphNames:
        data = data.applymap(shortenGraphName)
    return data

def savefig(fig, path):
    dirc = os.path.dirname(path)
    if not (os.path.exists(dirc) and os.path.isdir(dirc)):
        os.makedirs(dirc)
    fig.savefig(path)

#####################################################################################
## plotting stuff
def SetPueschelStyle(axes):
    """Set a graph style like prof. pueschel.
    
    Style includes a grey backgroud, a horizontal grid with
    white lines."""
    if isinstance(axes, pyplot.Figure):
        axes = axes.gca()
    if not isinstance(axes, pyplot.Axes):
        raise ValueError("axes must be pyplot.Figure or pyplot.Axes")

    axes.set_facecolor('xkcd:light grey')
    for spine in ['top', 'right', 'bottom', 'left']:
        axes.spines[spine].set_visible(False)
    axes.grid(which='major', axis='y', color='w',
        linestyle='-', linewidth=2)
    axes.tick_params(axis='y', length=0)

def PlotRuntime(threads, time, figure=None, title_suffix=None, pltkwargs=dict(), **kwargs):
    data = pd.DataFrame()
    data['threads'] = threads
    data['trial_time'] = time
    
    if figure == None:
        figure = pyplot.figure()
    ax = figure.gca()
    pooling = 'mean'
    if "pooling" in kwargs:
        pooling = kwargs["pooling"]
    if pooling == 'mean':
        algTime = data.groupby(['threads']).mean()
    elif pooling == 'median':
        algTime = data.groupby(['threads']).median()
    else:
        raise ValueError("unknown pooling strategy")

    ax.plot(algTime.index, algTime['trial_time'], **pltkwargs)
    ax.set_xlabel('# threads')
    ax.set_ylabel('Runtime [s]')

    if title_suffix == None:
        ax.set_title('Runtime')
    else:
        ax.set_title('Runtime [{}]'.format(title_suffix))

    return figure, ax

def PlotSpeedup(threads, time, figure=None, title_suffix=None, pltkwargs=dict(), **kwargs):
    data = pd.DataFrame()
    data['threads'] = threads
    data['trial_time'] = time

    if figure == None:
        figure = pyplot.figure()
    ax = figure.gca()

    pooling = 'mean'
    if "pooling" in kwargs:
        pooling = kwargs["pooling"]
    
    if pooling == 'mean':
        algTime = data.groupby(['threads']).mean()
    elif pooling == 'median':
        algTime = data.groupby(['threads']).median()
    else:
        raise ValueError("unknown pooling strategy")

    baseline = algTime.iloc[0]['trial_time']
    algTime['speedup'] = baseline/algTime['trial_time']

    ax.plot(algTime.index, algTime['speedup'], **pltkwargs)
    ax.set_xlabel('# threads')
    ax.set_ylabel('Speedup [1]')

    if title_suffix == None:
        ax.set_title('Speedup')
    else:
        ax.set_title('Speedup [{}]'.format(title_suffix))
    
    return figure, ax

def PlotPreprocessingInfo(pp_method, pp_time, time, no_pp_method, threads=None, 
    title_suffix = None, pltkwargs=dict(), **kwargs):
    data = pd.DataFrame()
    data['pp_method'] = pp_method
    data['pp_time'] = pp_time
    data['trial_time'] = time

    if threads is not None:
        data['threads'] = threads
        maxthread = data['threads'].max()
        data = data[ data['threads'] == maxthread ]
    
    figure = pyplot.figure()
    ax1, ax2 = figure.subplots(1,2)

    pooling = 'mean'
    if 'pooling' in kwargs:
        pooling = kwargs['pooling']
    
    if pooling == 'mean':
        ppTimes = data.groupby('pp_method').mean()
    elif pooling == 'median':
        ppTimes = data.groupby('pp_method').median()
    else:
        raise ValueError('Unknown pooling method')

    if pooling == 'mean':
        nopptime = data[ data['pp_method'] == no_pp_method ]['trial_time'].mean()
    elif pooling == 'median':
        nopptime = data[ data['pp_method'] == no_pp_method]['trial_time'].median()
    else:
        raise ValueError("Unknown pooling method")

    ax1.bar(ppTimes.index, ppTimes['pp_time'])
    ax1.set_xticklabels(ppTimes.index, rotation=60, ha='right')
    ax1.set_ylabel("time [s]")
    ax1.set_title('Preprocessing time')

    ax2.axhline(y=nopptime, color='c')
    ax2.bar(ppTimes.index, ppTimes['trial_time'])
    ax2.set_xticklabels(ppTimes.index, rotation=60, ha='right')
    ax2.set_ylabel("time [s]")
    ax2.set_title('Runtime')

    def percentage(y):
        return 100*y/nopptime
    
    def invpercentage(y):
        return y*nopptime/100

    ax2y2 = ax2.secondary_yaxis('right', functions=(percentage, invpercentage) )
    ax2y2.set_ylabel('percentage [%]')

    if title_suffix == None:
        figure.suptitle('Impact of preprocessing')
    else:
        figure.suptitle('Impact of preprocessing [{}]'.format(title_suffix))
    
    return figure, ax1, ax2, ax2y2

def PlotPreprocessingImpact(pp_method, time, no_pp_method, threads=None, 
    ax=None, pltkwargs=dict(), **kwargs):
    data = pd.DataFrame()
    data['pp_method'] = pp_method
    data['trial_time'] = time

    if threads is not None:
        data['threads'] = threads
        maxthread = data['threads'].max()
        data = data[ data['threads'] == maxthread ]

    figure = None
    if ax is None:
        figure = pyplot.figure()
        ax = figure.gca()

    pooling = 'mean'
    if 'pooling' in kwargs:
        pooling = kwargs['pooling']

    if pooling == 'mean':
        ppTimes = data.groupby('pp_method').mean()
    elif pooling == 'median':
        ppTimes = data.groupby('pp_method').median()
    else:
        raise ValueError('Unknown pooling method')

    if pooling == 'mean':
        nopptime = data[ data['pp_method'] == no_pp_method ]['trial_time'].mean()
    elif pooling == 'median':
        nopptime = data[ data['pp_method'] == no_pp_method]['trial_time'].median()
    else:
        raise ValueError("Unknown pooling method")

    ax.axhline(y=nopptime, color='c')
    ax.bar(ppTimes.index, ppTimes['trial_time'])
    ax.set_xticklabels(ppTimes.index, rotation=60, ha='right')
    ax.set_ylabel("time [s]")
    ax.set_title('Runtime')

    def percentage(y):
        return 100*y/nopptime
    
    def invpercentage(y):
        return y*nopptime/100

    axy2 = ax.secondary_yaxis('right', functions=(percentage, invpercentage) )
    axy2.set_ylabel('percentage [%]')

    return ax, axy2, figure

def PlotPreprocessingRuntime(pp_method, pp_time, threads=None, ax=None, pltkwargs=dict(), **kwargs):
    data = pd.DataFrame()
    data['pp_method'] = pp_method
    data['pp_time'] = pp_time

    if threads is not None:
        data['threads'] = threads
        maxthread = data['threads'].max()
        data = data[ data['threads'] == maxthread]

    figure = None
    if ax is None:
        figure = pyplot.figure()
        ax = figure.gca()

    pooling = 'mean'
    if 'pooling' in kwargs:
        pooling = kwargs['pooling']
    
    if pooling == 'mean':
        ppTimes = data.groupby('pp_method').mean()
    elif pooling == 'median':
        ppTimes = data.groupby('pp_method').median()
    else:
        raise ValueError('Unknown pooling method')

    ax.bar(ppTimes.index, ppTimes['pp_time'])
    ax.set_xticklabels(ppTimes.index, rotation=60, ha='right')
    ax.set_ylabel("time [s]")
    ax.set_title('Preprocessing time')

    return ax, figure

#################################################################################################
## example on how to use plotting routines

def plot_kclique(datapath, outputpath, scaling, preproc):
    # read data
    data = readData(datapath)

    # name columns for usability
    if data.shape[-1] == 12:
        # we have verifier data
        columns = {
            0:'algorithm', 
            1:'clique', 
            2:'iters', 
            3:'threads', 
            4:'graph', 
            5:'trial_time',
            6:'passmark', 
            7:'verify_time', 
            8:'pp_time', 
            9:'parallelization', 
            10:'algvariant',
            11:'pp_method'
        }
    elif data.shape[-1] == 10:
        columns = {
            0:'algorithm', 
            1:'clique', 
            2:'iters', 
            3:'threads', 
            4:'graph', 
            5:'trial_time',
            6:'pp_time', 
            7:'parallelizaton', 
            8:'algvariant', 
            9:'pp_method'
        }
    else:
        raise ValueError("Please define column-names")

    data = data.rename(columns=columns)

    data.loc[:,'algorithm'] = data['algorithm'].apply(lambda x :
        re.sub(r'Parallel_(degree|deg|id)', 'Parallel', x))

    for clique, cgroup in data.groupby(['clique']):
        for graph, graphgroup in cgroup.groupby(['graph']):
            for alg, algroup in graphgroup.groupby(['algorithm']):
                name = re.sub('clique_count_danisch_', '', alg)
                if preproc:
                    figure = pyplot.figure()
                    ax1, ax2 = figure.subplots(1,2)
                    ax1, _ = PlotPreprocessingRuntime(algroup['pp_method'], algroup['pp_time'],
                        threads=algroup['threads'], ax=ax1)
                    ax2, _, _ = PlotPreprocessingImpact(algroup['pp_method'], algroup['trial_time'], 
                        'id', threads=algroup['threads'],ax=ax2)

                    title_suffix = 'KClique: {} ({})'.format(name, graph)
                    figure.suptitle('Impact of preprocessing [{}]'.format(title_suffix), y=1.1)

                    try:
                        savefig(figure, outputpath / graph / (str(clique)+'clique') / ('ppInfo_'+name+'.svg'))
                    except Exception as e:
                        print("---------------------------------------------------")
                        print("Error when trying to save {}".format( Path(graph) / (str(clique)+'clique') / ('ppInfo_'+name+'.svg') ))
                        print("Error-Message: ")
                        print(str(e))
                        print("---------------------------------------------------")

                    pyplot.close(figure)
                    print('Kclique <> Graph: {}, Alg: {} --> Preproc Plotting done'.format(graph, name))

                if scaling:
                    sufig = None
                    rtfig = None
                    for pp, ppgroup in algroup.groupby(['pp_method']):
                        rtfig, rtax = PlotRuntime(ppgroup['threads'], ppgroup['trial_time'], figure=rtfig, pltkwargs={'label':pp},
                                                title_suffix=name+' ({})'.format(graph))
                        sufig, suax = PlotSpeedup(ppgroup['threads'], ppgroup['trial_time'], figure=sufig, pltkwargs={'label':pp},
                                                title_suffix=name+' ({})'.format(graph))

                    SetPueschelStyle(rtax)
                    SetPueschelStyle(suax)
                    rtax.legend(loc='upper right')
                    suax.legend(loc='upper left')

                    try:
                        savefig(rtfig, outputpath / graph / (str(clique)+'clique') / ('runtime_'+name+'_'+graph+'.svg'))
                    except Exception as e:
                        print("---------------------------------------------------")
                        print("Error when trying to save {}".format( Path(graph) / (str(clique)+'clique') / ('runtime_'+name+'.svg')))
                        print("Error-Message: ")
                        print(str(e))
                        print("---------------------------------------------------")
                    try:
                        savefig(sufig, outputpath / graph/ (str(clique)+'clique') / ('speedup_'+name+'.svg'))
                    except Exception as e:
                        print("---------------------------------------------------")
                        print("Error when trying to save {}".format( Path(graph) / (str(clique)+'clique') / ('speedup_'+name+'.svg')))
                        print("Error-Message: ")
                        print(str(e))
                        print("---------------------------------------------------")

                    pyplot.close(rtfig)
                    pyplot.close(sufig)
                    print("Kclique <> Graph: {}, Alg: {} ---> Scaling Plotting done".format(graph, name))

def plot_bronkerbosch(datapath, outputpath, scaling, preproc):
    data = readData(datapath)
    columns = {
        0: 'algorithm',
        1: 'iter',
        2: 'threads',
        3: 'graph',
        4: 'trial_time',
        5: 'algtype',
        6: 'algvariant',
        7: 'subvariant'
    }

    data = data.rename(columns=columns)
    # plot preprocessing info
    for graph, ggroup in data.groupby(['graph']):
        if preproc:
            preprocDf = ggroup[ ggroup[ 'algtype'] == 'preproc' ].copy()
            preprocDf['pp_method'] = preprocDf['algvariant'] + '_'+ preprocDf['subvariant']
            preprocDf.loc[:,'pp_method'] = preprocDf['pp_method'].apply(lambda x:
                re.sub('sequential', 'seq', x))
            preprocDf.loc[:,'pp_method'] = preprocDf['pp_method'].apply(lambda x:
                re.sub('parallel', 'par', x))

            bk = ggroup[ ggroup['algtype'] == 'bk' ].copy()
            bk['pp_method'] = bk['algvariant'] + '_' + bk['subvariant']
            bk.loc[:,'pp_method'] = bk['pp_method'].apply(lambda x:
                re.sub('_degeneracy','_deg', x))
            bk.loc[:,'pp_method'] = bk['pp_method'].apply(lambda x:
                re.sub('_direct', '_dir', x))
            bk.loc[:,'pp_method'] = bk['pp_method'].apply(lambda x:
                re.sub('subgraphing', 'subgraph', x))

            figure = pyplot.figure()
            ax1, ax2 = figure.subplots(1,2)

            ax1, _ = PlotPreprocessingRuntime(preprocDf['pp_method'], preprocDf['trial_time'],
                threads=preprocDf['threads'], ax=ax1)
            ax2, _, _ = PlotPreprocessingImpact(bk['pp_method'], bk['trial_time'], 
                'subgraph_dir', threads=bk['threads'],ax=ax2)

            title_suffix = 'BK ({})'.format(graph)
            figure.suptitle('Impact of preprocessing [{}]'.format(title_suffix), y=1.1)

            try:
                savefig(figure, outputpath / graph / ('ppInfo.svg'))
            except Exception as e:
                print("---------------------------------------------------")
                print("Error when trying to save: {}".format( Path(graph) / 'ppInfo.svg'))
                print("Error-Message:")
                print(str(e))
                print("---------------------------------------------------")
            pyplot.close(figure)
            print('Bronkerbosch <> Graph: {} --> Preprocessing plotting done'.format(graph))

        if scaling:
            for alg, algroup in bk.groupby(['algvariant']):
                sufig = None
                rtfig = None
                for pp, ppgroup in algroup.groupby(['subvariant']):
                    rtfig, rtax = PlotRuntime(ppgroup['threads'], ppgroup['trial_time'],
                        figure=rtfig, pltkwargs={'label':pp},
                        title_suffix=alg+' ({})'.format(graph))
                    sufig, suax = PlotSpeedup(ppgroup['threads'], ppgroup['trial_time'],
                        figure=sufig, pltkwargs={'label':pp},
                        title_suffix=alg+' ({})'.format(graph))
                
                SetPueschelStyle(rtax)
                SetPueschelStyle(suax)
                rtax.legend(loc='upper right')
                suax.legend(loc='upper left')

                try: 
                    savefig(rtfig, outputpath / graph / ('runtime_'+alg+'.svg'))
                except Exception as e:
                    print("---------------------------------------------------")
                    print("Error when trying to save: {}".format( Path(graph) / 'runtime_'+alg+'.svg'))
                    print("Error-Message:")
                    print(str(e))
                    print("---------------------------------------------------")
                try:
                    savefig(sufig, outputpath / graph / ('speedup_'+alg+'.svg'))
                except Exception as e:
                    print("---------------------------------------------------")
                    print("Error when trying to save: {}".format( Path(graph) / 'speedup_'+alg+'.svg'))
                    print("Error-Message:")
                    print(str(e))
                    print("---------------------------------------------------")
                pyplot.close(sufig)
                pyplot.close(rtfig)
        print('Bronkerbosch <> Graph: {} --> Scaling plotting done'.format(graph))


def help():
    print('Usage: {} algtype [scaling|preproc] [options]'.format(sys.argv[0]))
    print("  algtype               currently either 'bronkerbosch' or 'kclique'")
    print("  [scaling|preproc]     define either 'scaling' or 'preproc' to plot only")
    print('                        scalability (speedup, runtime) or preproessing')
    print('                        info respectively. Define nothing to plot both.')
    print('  Options:')
    print('  -o, --output= <dir>    directory where to save the output')
    print('  -i, --input= <file>    input data file')
    print('  -h, --help            print this help')
    print()
    print()

if __name__ == '__main__':
    ## handle commmand line parameters
    if len(sys.argv) < 2:
        print('Not enough parameters. Aborting')
        help()
        sys.exit(1)

    algtype = sys.argv[1]
    if algtype == '-h' or algtype == '--help':
        help()
        sys.exit(0)

    functionname = 'plot_' + algtype.lower()
    argIdx = 3
    scaling = True
    preproc = True
    if sys.argv[2].lower() == 'scaling':
        preproc = False
    elif sys.argv[2].lower() == 'preproc':
        scaling = False
    else:
        argIdx = 2

    try:
        opts, _ = getopt.getopt(sys.argv[argIdx:],
            "i:o:h", ["help", "input=", "output="])
    except getopt.GetoptError:
        print(' --  Invalid Argument  -- ')
        help()
        sys.exit(2)
    
    dpath = None
    opath = None

    for opt, arg in opts:
        if opt in {'-h', '--help'}:
            help()
            sys.exit()
        elif opt in {'-i', '--input'}:
            dpath = arg
        elif opt in {'-o', '--output'}:
            opath = arg
    
    if dpath is None:
        print('No input specified. Aborting.')
        sys.exit(1)
    if opath is None:
        print("No output directory specified. Using default-value:  './plot'")
        opath = 'plot'

    opath = Path(opath)
    dpath = Path(dpath)
    if os.path.isfile(opath) or not os.path.exists(opath):
        os.makedirs(opath)

    plot_algorithm = locals()[functionname]
    plot_algorithm(dpath, opath, scaling, preproc)    
