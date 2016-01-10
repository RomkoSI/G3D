import datetime, os, sys, subprocess, csv
from .help import maybeWarn
from .variables import State
from .utils import *
from .copyifnewer import excludeFromCopying

# Information about the project for development tracing purposes.
# See processStatistics
class CompilationStatistics:
    statementCount = 0
    lineCount  = 0
    numFiles = 0
    dateTime = None
    user = None
    routines = 0
    classes = 0
    dataBytes = 0
    numDataFiles = 0

    # Non-doxygen comments
    commentLines = 0

    # .htm and .html files in the doc-files directory plus doxygen comments
    documentationLines = 0

    # functions, macros, and types
    routines = 0

    classes = None

    # Produces a list of column entries that are all strings, suitable for writing to a CSV file
    def toRow(self):
        return [str(x) for x in
                [self.dateTime.strftime('%d-%b-%Y %H:%M'), self.numFiles, self.statementCount, self.lineCount, self.commentLines, self.documentationLines, self.numDataFiles, self.dataBytes, self.user, self.routines, self.classes]]

    # Parses the output of toRow into a valid CompilationStatistics instance
    @classmethod
    def fromRow(cls, row):
        s = cls()
        s.dateTime           = datetime.datetime.strptime(row[0], '%d-%b-%Y %H:%M')
        s.numFiles           = int(row[1])
        s.statementCount     = int(row[2])
        s.lineCount          = int(row[3])
        s.commentLines       = int(row[4])
        s.documentationLines = int(row[5])
        s.numDataFiles       = int(row[6])
        s.dataBytes          = int(row[7])
        s.user               = row[8]
        s.routines           = int(row[9])
        s.classes            = int(row[10])
        return s

    @classmethod
    def headerString(cls):
        return '"Date","Source Files","Statements","Lines","Comment Lines","Documentation Lines","Data Files","Data Bytes","User","Routines","Classes"\n'



# Recursively walks the data-files directory
# and returns a tuple of the number of files and their size
def computeDataSize():
    fileCount = 0
    byteCount = 0

    for path, subdirs, files in os.walk('data-files'):
        if (excludeFromCopying.search(betterbasename(path)) != None):
            # Don't recurse into subdirectories of excluded directories
            del subdirs[:]
        else:
            for file in files:
                if (not file.endswith('~') and not file.startswith('#') and not file.endswith('.dll') and
                    not file.endswith('.dylib') and not file.endswith('.so')):
                    fileCount += 1
                    byteCount += os.path.getsize(os.path.join(path, file))

    return (byteCount, fileCount)


# Returns an array of CompilationStatistics
def loadCompilationStatisticsArrayFromCSV(logfile):
    f = open(logfile, 'rt')
    iterator = iter(csv.reader(f))
    # remove the header line
    next(iterator)
    data = [Stats.fromRow(row) for row in iterator]
    f.close()
    return data


def graphCompilationStatisticsArray(statArray):
    return None

def abbreviateNumber(num):
    if num < 1000:
        return str(num)
    elif num < 1000000:
        return str(num // 1000) + "k"
    else:
        return str(num // 1000000) + "m"

def generateStatistics(state):
    stats = CompilationStatistics()

    files = listCFiles('', state.excludeFromCompilation, True)
    # equivalent to:
    #   wc -l <files> | tail -1
    #   grep ';' <files> | wc -l | tail -1
    stats.statementCount = 0
    stats.lineCount = 0
    stats.numFiles = len(files)

    for file in files:
        x = readFile(file)
        stats.statementCount += x.count(';')
        stats.lineCount += x.count('\n')

        (comments, docLines) = countComments(x)
        stats.commentLines += comments
        stats.documentationLines += docLines

    stats.dateTime = datetime.datetime.now()

    # Round off dateTime to the nearest minute
    stats.dateTime = datetime.datetime(stats.dateTime.year, stats.dateTime.month, stats.dateTime.day, stats.dateTime.hour, stats.dateTime.minute)

    stats.user = os.environ.get('USER')

    # count routines. -t will make it count typedefs and enums, but that throws
    # off our average statement count

    # Do we have Exuberant Ctags?
    try:
        hasExuberant = (subprocess.check_output('ctags --version', shell=True)).decode("utf-8").startswith("Exuberant")
    except:
        maybeWarn("Cannot run ctags, which is necessary for statistics generation.\n Run Xcode once as an administrator to initialize command line tools.", state)
        sys.exit(1)
    if hasExuberant:
        stats.routines = int(subprocess.check_output('ctags -xw --c++-kinds=f ' + (' '.join(files)) + ' | wc -l', shell=True))
        stats.classes  = int(subprocess.check_output('ctags -xw --c++-kinds=c ' + (' '.join(files)) + ' | wc -l', shell=True))
    else:
        maybeWarn('Using old ctags instead of exuberant ctags. Upgrade with "sudo port install ctags" (http://ctags.sf.net/) for better summary information.', state)
        stats.routines = int(subprocess.check_output('ctags -xw ' + (' '.join(files)) + ' | wc -l', shell=True))
        stats.classes = 0

    (stats.dataBytes, stats.numDataFiles) = computeDataSize()

    return stats


# Computes development progress metrics. Returns a string briefly
# describing the result.
def processStatistics(state):
    if verbosity >= VERBOSE:
        print('Computing statistics...')

    stats = generateStatistics(state)

    logfile = 'ice-stats.csv'

    if not os.path.exists(logfile):
        writeFile(logfile, CompilationStatistics.headerString())

    infoRow = stats.toRow()

    f = open(logfile, 'rt')
    iterator = iter(csv.reader(f))

    # copy the header
    data = [next(iterator)]

    # Find the last entry by this user
    lastDateTime         = datetime.datetime(1980, 1, 1, 0, 0)
    secondToLastDateTime = datetime.datetime(1980, 1, 1, 0, 0)
    lastIndex = -1

    # Counter for the row iterator
    for i, row in enumerate(iterator, start = 1):
        # Copy this result to the output
        data += [row]

        # See if this entry is for today by this user
        if row[8] == stats.user:
            entryDateTime = datetime.datetime.strptime(row[0], '%d-%b-%Y %H:%M')
            if entryDateTime > lastDateTime:
                secondToLastDateTime = lastDateTime
                lastDateTime = entryDateTime
                lastIndex = i
    f.close()

    if lastIndex != -1:
        halfHour = datetime.timedelta(0, 0, 0, 0, 30)
        lastEntryIsWithinHalfAnHour = (lastDateTime > (stats.dateTime - halfHour))

        # Insert the new entry
        if lastEntryIsWithinHalfAnHour and ((lastDateTime - secondToLastDateTime) < halfHour):
            # Overwrite the previous entry for this half-hour by this user
            data[lastIndex] = infoRow
        else:
            # Insert right after the last date modified BY THIS USER,
            # conveniently avoiding modifying the same line if two
            # users are compiling on different machines and will later
            # try to merge their code
            data.insert(lastIndex + 1, infoRow)
    else:
        # Append a new entry because this user has never compiled before
        data += [infoRow]

    f = open(logfile, 'wt')
    writer = csv.writer(f)
    for row in data:
        writer.writerow(row)
    f.close()

    if verbosity >= VERBOSE:
        print('Done computing statistics.')

    return '\nCompiled %(files)s files, %(statements)s statements, %(comments)s comment lines, %(classes)s %(routines)s routines of average length %(avg)d statements\n' % \
              {'files' : abbreviateNumber(stats.numFiles),
               'statements' : abbreviateNumber(stats.statementCount),
               'comments' : abbreviateNumber(stats.commentLines + stats.documentationLines),
               'routines' : abbreviateNumber(stats.routines),
               'classes'  : ('' if (stats.classes == None) else (str(stats.classes) + (' class,' if (stats.classes == 1) else ' classes,'))),
               'avg': int(0.5 + stats.statementCount / max(1, stats.routines))}
