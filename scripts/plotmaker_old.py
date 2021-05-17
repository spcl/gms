#!/bin/python3

##################################################################################################################
##
## usage info is at the bottom, scroll down to main part for how-to
##
##################################################################################################################

import os, re
import numpy as np
import pandas as pd
from matplotlib import pyplot

def defaultOptions():
    return {
        "pueschel":False, 
        "pooling":"mean", 
        "saveas":None,
        "noPP":"id"
        }

def shortenGraphName(arg):
    name = basename(arg)
    pattern = 'd-abc'
    return name[:len(pattern)]

def basename(arg):
    """Try to extract a basename from the provided argument."""
    if not isinstance(arg, str):
        arg = str(arg)
    try:
        name = os.path.basename(arg)
    except:
        name = arg

    fileExtensions = ['el', 'txt', 'dat', 'csv', 'exe', 'out']
    for ext in fileExtensions:
        name = re.sub(r'\.'+'{}$'.format(ext), '', name)
    name = re.sub(r'\.', '_', name)
    return name

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


def PlotRuntime(data, figure=None, graph=None, options="default"):
    """"Plots the runtime of a single algorithm.
    
    Assumes data is pandas DataFrame."""
    # check columns
    for col in ["threads", "trial_time"]:
        if not col in data.columns:
            raise ValueError("columns require label '{}'".format(col))

    if figure==None:
        figure = pyplot.figure()
    ax = figure.gca()

    if options == "default":
        options = defaultOptions()

    if options["pooling"]=='mean':
        algTime = data.groupby(['threads']).mean()
    elif options["pooling"]=='median':
        algTime = data.groupby(['threads']).median()
    else:
        raise ValueError("Unknown pooling strategy")
    
    if "algorithm" in options:
        algorithm = options["algorithm"]
    elif "algorithm" in data.columns:
        algorithm = data["algorithm"].iloc[0]
    else:
        algorithm= None

    if algorithm==None:
        ax.plot(algTime.index, algTime['trial_time'])
    else:
        ax.plot(algTime.index, algTime['trial_time'], label=algorithm)
        ax.legend(loc='upper right')
    ax.set_xlabel('# threads')
    ax.set_ylabel('Runtime [s]')
    if graph==None:
        title_add=''
    else:
        title_add=' [{}]'.format(graph)
    ax.set_title('Strong Scaling' + title_add)

    if options["pueschel"]:
        SetPueschelStyle(ax)
    return figure

def PlotSpeedup(data, figure=None, graph=None, options="default"):
    """Plot the speedup of a single algorithm.
    
    Assumes data is pandas DataFrame with columns
    'algorithm' (used as label), 'time' and 'threads'."""
    # check columns
    for col in ["threads", "trial_time"]:
        if not col in data.columns:
            raise ValueError("columns require label '{}'".format(col))

    if figure==None:
        figure = pyplot.figure()
    ax = figure.gca()

    if options == "default":
        options = defaultOptions()

    if options["pooling"]=='mean':
        algTime = data.groupby(['threads']).mean()
    elif options["pooling"]=='median':
        algTime = data.groupby(['threads']).median()
    else:
        raise ValueError("Unknown pooling strategy")

    baseline = algTime.iloc[0]['trial_time']
    algTime['speedup'] = baseline/algTime['trial_time']

    if "algorithm" in options:
        algorithm = options["algorithm"]
    elif "algorithm" in data.columns:
        algorithm = data["algorithm"].iloc[0]
    else:
        algorithm= None

    if algorithm==None:
        ax.plot(algTime.index, algTime['speedup'])
    else:
        ax.plot(algTime.index, algTime['speedup'], label=algorithm)
        ax.legend(loc='upper left')
    ax.set_xlabel('# threads')
    ax.set_ylabel('Speedup [1]')
    if graph == None:
        title_add=''
    else:
        title_add=' [{}]'.format(graph)
    ax.set_title('Speedup'+title_add)

    if options["pueschel"]:
        SetPueschelStyle(ax)
    return figure

def PlotPreprocessing(data, graph=None, options="default"):
    """Plot the impact of the preprocessing.
    
    Arguments:
    data: pandas DataFrame. Has columns 'pp_method', 'pp_time' and 'trial_time'
    graph: name of graph (relevant for title)
    options: dictionary with plotting options.

    Returns figure, ax1, ax2, ax2y2 where ax1 and ax2 are the axes of th respective subplots. ax2y2 is the secondary axis of
    the second plot.
    """
    # check dict
    for col in ["pp_method", "pp_time", "trial_time"]:
        if not col in data.columns:
            raise ValueError("columns require label :'{}'".format(col))
    if options == "default":
        options = defaultOptions()

    if "threads" in data.columns:
        maxthread = data["threads"].max()
        data = data[ data["threads"] == maxthread ]

    figure = pyplot.figure()
    ax1, ax2 = figure.subplots(1,2)
    # do preprocessing time
    if options["pooling"] == 'mean':
        ppTimes = data[['pp_method', 'pp_time', 'trial_time']].groupby(['pp_method']).mean()
    elif options["pooling"] == 'median':
        ppTimes = data[['pp_method', 'pp_time', 'trial_time']].groupby(['pp_method']).median()
    else:
        raise ValueError("Unknown pooling method")
    
    ax1.bar(ppTimes.index, ppTimes['pp_time'])
    ax1.set_xticklabels(ppTimes.index, rotation=60)
    ax1.set_ylabel("time [s]")
    ax1.set_title('Preprocessing time')
    
    # do runtime comp
    if options["pooling"] == 'mean':
        nopptime = data[ data['pp_method'] == options['noPP']]['trial_time'].mean()
    elif options["pooling"] == 'median':
        nopptime = data[ data['pp_method'] == options['noPP']]['trial_time'].median()
    else:
        raise ValueError("Unknown pooling method")

    ax2.axhline(y=nopptime, color='c')
    ax2.bar(ppTimes.index, ppTimes['trial_time'])
    ax2.set_xticklabels(ppTimes.index, rotation=0)
    ax2.set_ylabel("time [s]")

    def percentage(y):
        return 100*y/nopptime
    
    def invpercentage(y):
        return y*nopptime/100

    ax2y2 = ax2.secondary_yaxis('right', functions=(percentage, invpercentage) )
    ax2y2.set_ylabel('percentage [%]')

    if "algorithm" in options:
        algorithm = options["algorithm"]
    elif "algorithm" in data.columns:
        algorithm = data["algorithm"].iloc[0]
    else:
        algorithm= None

    if algorithm==None:
        ax2.set_title('Runtime')
    else:
        ax2.set_title('Runtime: {}'.format(algorithm))
    
    if graph==None:
        figure.suptitle('Impact of preprocessing')
    else:
        figure.suptitle('Impact of preprocessing [{}]'.format(graph))

    figure.SubplotParams(wspace=0.2)
    return figure, ax1, ax2, ax2y2

def createPlots(data, options, graph=None, runTimeFig=None, speedupFig=None):
    
    if options == "default":
        options = defaultOptions()

    if "algorithm" in options:
        algorithm = options["algorithm"]
    elif "algorithm" in data.columns:
        algorithm = data["algorithm"].iloc[0]
    else:
        algorithm= None

    # configure what to do
    makeRuntime = True
    for col in ["threads", "trial_time"]:
        if not col in data.columns:
            makeRuntime = False
    
    makeSpeedup = True
    for col in ["threads", "trial_time"]:
        if not col in data.columns:
            makeSpeedup = False
    
    makePP = True
    for col in ["pp_method", "pp_time", "trial_time"]:
        if not col in data.columns:
            makePP = False

    if not options["saveas"] == None:
        if graph==None:
            suffix="_{}.{}".format(algorithm, options["saveas"])
        else:
            suffix="_{}_{}.{}".format(algorithm, graph, options["saveas"])
    else:
        suffix=""    

    if makeRuntime:
        runFig = PlotRuntime(data, runTimeFig, graph, options)
        if not options["saveas"] == None:
            runFig.savefig("runtime"+suffix)
    else:
        runFig = None
    
    if makeSpeedup:
        speedFig = PlotSpeedup(data, speedupFig, graph, options)
        if not options["saveas"] == None:
            speedFig.savefig("speedup"+suffix)
    else:
        speedFig = None

    if makePP:
        ppFig = PlotPreprocessing(data, graph, options)
        if not options["saveas"] == None:
            ppFig.savefig("pp"+suffix)
    else:
        ppFig = None

    return  runFig, speedFig, ppFig

def get_kc_columns(pp=True):
    columns = dict()
    columns = dict()
    columns["algorithm"] = 0
    columns["graphs"] = 4
    columns["trial_time"] = 5
    columns["threads"] = 3
    if pp:
        columns["pp_method"] = 9
        columns["pp_time"] = 6
    

    return columns



###############################################################################################################
## 
## main part: function for plotting
##
## customize here
##
###############################################################################################################

def plotSingleAlgorithm():
    """This function can plot Runtime, Speedup and preprocessing info for a single Algorithm."""
    # read in data from relevant file
    filepath = "/home/yschaffner/Dokumente/ETH/sa_graphs/cscs_output/20200109_data.csv"
    rawdata = pd.read_csv(filepath, sep=" ", header=None)

    # set the columns of the respective values for the current algorithm
    # set only what you need/want to plot
    # to plot runtime/speedup you need: "trial_time", "threads"
    # to plot preprocessing info you need: "pp_method", "pp_time"
    columns = dict()
    ## need to include in dict:
    columns["algorithm"] = 0   #Column with executable name (always column 0)
    columns["graphs"] = 2      #Column where the input graph is specified

    ## optional to include in dict:
    columns["trial_time"] = 5  #Column with the trial time
    columns["threads"] = 4     #Column with the number of used threads

    columns["pp_method"] = 7   #Column with the applied preprocessing method
    columns["pp_time"] = 6     #Column with the time measurement for the preprocessing

    

    # set columns explicitly (as above) or get from predefined function (as below)
    columns = get_kc_columns(True)

    
    ##################################################################################################
    ## do some basic preprocessing:
    # shorten file names, change name patterns of algorithm, graphs, ...
    # select relevant rows,...
    data = rawdata.rename(columns={ v:k for k,v in columns.items()})
    data.loc[:,"algorithm"] = data["algorithm"].apply(basename)

    

    ###################################################################################################
    ## define which algorithm you wish to plot (name the executble)
    selectedAlgorithm = "clique_count_danisch_nodeParallel"

    algData = data[ data["algorithm"] == selectedAlgorithm].copy()
    # => apply some cosmetic changes. Perform as suits you best
    algData.loc[:,"algorithm"] = algData["algorithm"].apply(lambda arg : re.sub("clique_count_danisch", "cc", arg))
    algData.loc[:,"graphs"] = algData["graphs"].apply(shortenGraphName)    
    algData.loc[:,"pp_method"] = algData["pp_method"].apply(lambda arg: re.sub("degeneracy", "deg", arg))

    ## define options for plotting
    options = defaultOptions()
    options["pueschel"] = True #True: use pueschel graph style, False: use default style
    options["saveas"] = "png" #set to None to not save to disk

    # convert data types
    if "trial_time" in columns:
        algData.loc[:,"trial_time"] = algData["trial_time"].astype(float)
    if "pp_time" in columns:
        algData.loc[:,"pp_time"] = algData["pp_time"].astype(float)
    if "threads" in columns:
        algData.loc[:,"threads"] = algData["threads"].astype(int)
    graphGroups = algData.groupby(["graphs"])
    for key, group in graphGroups:
        createPlots(group, graph=key, options=options)

if __name__ == "__main__":
    ## to plot information of a single algorithm
    plotSingleAlgorithm()





