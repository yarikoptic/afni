#!/usr/bin/env python

# system libraries
import sys, os

# AFNI libraries
import option_list as OL
import afni_util as UTIL        # not actually used, but probably will be
import lib_subjects as SUBJ

# ----------------------------------------------------------------------
# globals

g_help_string = """
=============================================================================
gen_group_command.py    - generate group commands: 3dMEMA
                        - todo: 3dttest, 3dttest++, 3dANOVA2, 3dANOVA3

   This program is to assist in writing group commands.  The hardest part (or
   most tedious) is generally listing datasets and such, and that is the main
   benefit of using this program.

------------------------------------------
examples (by program)

   A. 3dMEMA

      Note: these commands apply to the sample group data under
            AFNI_data6/group_results.

      1. The most simple case, providing just the datasets.  The subject IDs
         will be extracted from the dataset names.  Since no sub-bricks are
         provided, the betas will be 0 and t-stats will be 1.

            gen_group_command.py -command 3dMEMA        \\
                                 -dsets OL*.HEAD

      2. This does not quite apply to AFNI_data6.  Assuming there are 2 group
         directories, write a 2-sample command.

            gen_group_command.py -command 3dMEMA        \\
                                 -dsets groupA/OL*.HEAD
                                 -dsets groupB/OL*.HEAD

      3. Specify the sub-bricks, to compare Vrel vs. Arel.

            gen_group_command.py -command 3dMEMA        \\
                                 -dsets OL*.HEAD        \\
                                 -set_labels Vrel Arel  \\
                                 -subs_betas 0 2        \\
                                 -subs_tstats 1 3

      4. Similar to 3, but more advanced.  Specify sub-bricks using the labels,
         specify a paired test, and add some extra options.

            gen_group_command.py -command 3dMEMA                            \\
                                 -type paired                               \\
                                 -dsets OL*.HEAD                            \\
                                 -set_labels Vrel Arel                      \\
                                 -subs_betas  'Vrel#0_Coef'  'Arel#0_Coef'  \\
                                 -subs_tstats 'Vrel#0_Tstat' 'Arel#0_Tstat' \\
                                 -options                                   \\
                                    -mask mask+tlrc -max_zeros 0.25 -jobs 2 \\
                                    -HKtest -model_outliers

         See "3dMEMA -help" for details on the given options.

------------------------------------------
terminal options:

   -help                     : show this help
   -hist                     : show module history
   -show_valid_opts          : list valid options
   -ver                      : show current version

required parameters:

   -command COMMAND_NAME     : resulting command, such as 3dttest++
   -dsets   datasets ...     : list of datasets
                                  (this option can be used more than once)

other options:

   -options OPT1 OPT2 ...       : list of options to pass along to result

        The given options will be passed directly to the resulting command.  If
        the -command is 3dMEMA, say, these should be 3dMEMA options.  This
        program will not evaluate or inspect the options, but will put them at
        the end of the command.

   -prefix PREFIX               : apply as COMMAND -prefix
   -set_labels LAB1 LAB2 ...    : labels corresponding to -dsets entries
   -subj_prefix PREFIX          : prefix for subject names (3dMEMA)
   -subj_suffix SUFFIX          : suffix for subject names (3dMEMA)
   -subs_betas B0 B1            : sub-bricks for beta weights (or similar)

        If this option is not given, sub-brick 0 will be used.  The entries
        can be either numbers or labels (which should match what is seen in
        the afni GUI, for example).

        If there are 2 -set_labels, there should be 2 betas (or no option).

   -subs_tstats T0 T1           : sub-bricks for t-stats (3dMEMA)

        If this option is not given, sub-brick 1 will be used.  The entries can
        be either numbers or labels (which should match what is seen in the
        afni GUI, for example).

        This option applies only to 3dMEMA currently, and in that case, its use
        should match that of -subs_betas.

        See also -subs_betas.

   -type TEST_TYPE              : specify the type of test to perform

        The test type may depend on the given command, but generally implies
        there are multiple sets of values to compare.  Currently valid tests
        are (for the given program):
       
          3dMEMA: paired, unpaired

        If this option is not applied, a useful default will be chosen.

   -verb LEVEL                  : set the verbosity level

-----------------------------------------------------------------------------
R Reynolds    October 2010
=============================================================================
"""

g_history = """
   gen_group_command.py history:

   0.0  Sep 09, 2010    - initial version
   0.1  Oct 25, 2010    - handle some 3dMEMA cases
"""

g_version = "gen_group_command.py version 0.1, Sep 9, 2010"


class CmdInterface:
   """interface class for getting commands from SubjectList class
   """

   def __init__(self, verb=1):
      # main variables
      self.status          = 0                       # exit value
      self.valid_opts      = None
      self.user_opts       = None

      # general variables
      self.command         = ''         # program name to make command for
      self.ttype           = None       # test type (e.g. paired)
      self.prefix          = None       # prefix for command result
      self.betasubs        = None       # list of beta weight sub-brick indices
      self.tstatsubs       = None       # list of t-stat sub-brick indices
      self.lablist         = None       # list of set labels

      self.subj_prefix     = ''         # prefix for each subject ID
      self.subj_suffix     = ''         # suffix for each subject ID
      self.verb            = verb

      # lists
      self.options         = []         # other command options
      self.slist           = []         # list of SubjectList elements
      self.dsets           = []         # list of lists of filenames

      # initialize valid_opts
      self.init_options()

   def show(self):
      print "----------------------------- setup -----------------------------"
      print "command          : %s" % self.command
      print "test type        : %s" % self.ttype
      print "prefix           : %s" % self.prefix
      print "beta sub-bricks  : %s" % self.betasubs
      print "tstat sub-bricks : %s" % self.tstatsubs
      print "label list       : %s" % self.lablist
      print "subject prefix   : %s" % self.subj_prefix
      print "subject suffix   : %s" % self.subj_suffix
      print "verb             : %s" % self.verb

      print "options          : %s" % self.options
      print "subject list(s)  : %s" % self.slist
      print "datasets         : %s" % self.dsets        # last

      if self.verb > 3:
         print "status           : %s" % self.status
         self.user_opts.show(mesg="user options     : ")
      print "-----------------------------------------------------------------"

   def init_options(self):
      self.valid_opts = OL.OptionList('valid opts')

      # terminal options
      self.valid_opts.add_opt('-help', 0, [],           \
                      helpstr='display program help')
      self.valid_opts.add_opt('-hist', 0, [],           \
                      helpstr='display the modification history')
      self.valid_opts.add_opt('-show_valid_opts', 0, [],\
                      helpstr='display all valid options')
      self.valid_opts.add_opt('-ver', 0, [],            \
                      helpstr='display the current version number')

      # required parameters
      self.valid_opts.add_opt('-command', 1, [], 
                      helpstr='specify the program used in the output command')
      self.valid_opts.add_opt('-dsets', -1, [], 
                      helpstr='specify a list of input datasets')

      # other options
      self.valid_opts.add_opt('-options', -1, [], 
                      helpstr='specify options to pass to the command')
      self.valid_opts.add_opt('-prefix', 1, [], 
                      helpstr='specify output prefix for the command')
      self.valid_opts.add_opt('-set_labels', -1, [], 
                      helpstr='list of labels for each set of subjects')
      self.valid_opts.add_opt('-subj_prefix', 1, [], 
                      helpstr='specify prefix for each subject ID')
      self.valid_opts.add_opt('-subj_suffix', 1, [], 
                      helpstr='specify suffix for each subject ID')
      self.valid_opts.add_opt('-subs_betas', -1, [], 
                      helpstr='beta weight sub-bricks, one per subject list')
      self.valid_opts.add_opt('-subs_tstats', -1, [], 
                      helpstr='t-stat sub-bricks, one per subject list')
      self.valid_opts.add_opt('-type', 1, [], 
                      helpstr='specify the test type (e.g. paired)')
      self.valid_opts.add_opt('-verb', 1, [], 
                      helpstr='set the verbose level (default is 1)')

      return 0

   def process_options(self, argv=sys.argv):

      # process any optlist_ options
      self.valid_opts.check_special_opts(argv)

      # process terminal options without the option_list interface
      # (so that errors are not reported)

      # if no arguments are given, apply -help
      if len(argv) <= 1 or '-help' in argv:
         print g_help_string
         return 0

      if '-hist' in argv:
         print g_history
         return 0

      if '-show_valid_opts' in argv:
         self.valid_opts.show('', 1)
         return 0

      if '-ver' in argv:
         print g_version
         return 0

      # ============================================================
      # read options specified by the user
      self.user_opts = OL.read_options(argv, self.valid_opts)
      uopts = self.user_opts            # convenience variable
      if not uopts: return 1            # error condition

      # ------------------------------------------------------------
      # require a list of files, at least

      # ------------------------------------------------------------
      # process options, go after -verb first

      val, err = uopts.get_type_opt(int, '-verb')
      if val != None and not err: self.verb = val

      for opt in uopts.olist:

         # main options
         if opt.name == '-command':
            val, err = uopts.get_string_opt('', opt=opt)
            if val == None or err: return 1
            self.command = val
            continue

         if opt.name == '-dsets':
            val, err = uopts.get_string_list('', opt=opt)
            if val == None or err: return 1
            self.dsets.append(val)      # allow multiple such options
            continue

         if opt.name == '-options':
            val, err = uopts.get_string_list('', opt=opt)
            if val == None or err: return 1
            self.options = val
            continue

         if opt.name == '-prefix':
            val, err = uopts.get_string_opt('', opt=opt)
            if val == None or err: return 1
            self.prefix = val
            continue

         if opt.name == '-set_labels':
            val, err = uopts.get_string_list('', opt=opt)
            if val == None or err: return 1
            self.lablist = val
            continue

         if opt.name == '-subj_prefix':
            val, err = uopts.get_string_opt('', opt=opt)
            if val == None or err: return 1
            self.subj_prefix = val
            continue

         if opt.name == '-subj_suffix':
            val, err = uopts.get_string_opt('', opt=opt)
            if val == None or err: return 1
            self.subj_suffix = val
            continue

         if opt.name == '-subs_betas':
            val, err = uopts.get_string_list('', opt=opt)
            if val == None or err: return 1
            self.betasubs = val
            continue

         if opt.name == '-subs_tstats':
            val, err = uopts.get_string_list('', opt=opt)
            if val == None or err: return 1
            self.tstatsubs = val
            continue

         if opt.name == '-type':
            val, err = uopts.get_string_opt('', opt=opt)
            if val == None or err: return 1
            self.ttype = val
            continue

         if opt.name == '-verb': continue       # already handled

         # general options

         # an unhandled option
         print '** option %s not yet supported' % opt.name
         return 1

      if self.verb > 2: self.show()

      return None

   def execute(self):

      if not self.ready_for_action(): return 1

      if self.verb > 1:
         print '-- make %s command with %d set(s) of dsets of length(s): %s' \
               % (self.command, len(self.dsets), 
                  ', '.join([str(len(dlist)) for dlist in self.dsets]) )

      # might deal with subject IDs and attributes later
      for dlist in self.dsets:
         slist = SUBJ.SubjectList(dset_l=dlist, verb=self.verb)
         slist.set_ids_from_dsets(prefix=self.subj_prefix,
                                  suffix=self.subj_suffix)
         self.slist.append(slist)

      cmd = None
      if self.command == '3dMEMA':
         cmd = self.get_mema_command()
      elif self.command == '3dttest':
         print '** 3dttest command not yet implemented'
      else:
         print '** unrecognized command: %s' % self.command

      if cmd == None: return 1
      cmd = UTIL.add_line_wrappers(cmd)

      print cmd

      # rcr - maybe write to file

   def get_mema_command(self):
      if len(self.slist) > 1: s2 = self.slist[1]
      else:                   s2 = None
      if (self.betasubs != None and self.tstatsubs == None) or \
         (self.betasubs == None and self.tstatsubs != None):
         print '** -subs_betas and -subs_tstats must be used together'
         return 1
      return self.slist[0].make_mema_command(set_labs=self.lablist,
                     bsubs=self.betasubs, tsubs=self.tstatsubs, subjlist2=s2,
                     prefix=self.prefix, ttype=self.ttype, options=self.options)

   def help_mema_command(self):
      helpstr = """
        3dMEMA command help:

           This is for help in deciding which MEMA command format to use, which
           command parameters are needed, and how a command would be organized.

           As with 3dttest, there are 3 basic ways to run 3dMEMA.

              1. as a one-sample test  (see example 1 from '3dMEMA -help')
              2. as a two-sample test  (see example 3 from '3dMEMA -help')
              3. as a paired test      (see example 2 from '3dMEMA -help')

           1. For the one-sample test, the required inputs are the datasets.
              It is best to also supply corresponding beta and t-stat sub-brick
              indexes or labels as well.

              minimum:

                 -dsets stats.*+tlrc.HEAD

              suggested:

                 -subs_betas 0 -subs_tstats 1
                    OR
                 -subs_betas  'Vrel#0_Coef' -subs_tstats 'Vrel#0_Tstat'

           2. 
      """

   def help_datasets(self):
      helpstr = """
           Dataset configuration and naming:
              When using this command generator, each subject should have all
              data (betas and t-stats) in a single dataset.  Dataset names
              should preferably be consistent, varying over only subject ID
              codes (this is not a requirement, but makes life easier).

              For example, this allows one to specify datasets at once (via a
              wildcard) and let the program sort it out.  Consider using:

                -dsets stats.*+tlrc.HEAD

              This will expand alphabetically, with subject IDs coming from
              the part of the dataset names that varies.

      """

   def ready_for_action(self):

      ready = 1

      if self.command == '':
         if self.verb > 0: print '** missing execution command'
         ready = 0

      if len(self.dsets) < 1:
         if self.verb > 0: print '** missing datasets for command'
         ready = 0

      return ready

   def init_from_file(self, fname):
      """load a 1D file, and init the main class elements"""

      self.status = 1 # init to failure
      adata = L1D.Afni1D(fname, name='1d_tool data', verb=self.verb)
      if not adata.ready:
         print "** failed to read 1D data from '%s'" % fname
         return 1

      if self.verb > 1: print "++ read 1D data from file '%s'" % fname

      self.ad = adata
      self.status = 0

      return 0

   def test(self, verb=3):
      print '------------------------ initial tests -----------------------'
      self.verb = verb
      # first try AFNI_data4, then regression data

      print '------------------------ reset files -----------------------'

      print '------------------------ should fail -----------------------'

      print '------------------------ more tests ------------------------'

      return None

def main():
   me = CmdInterface()
   if not me: return 1

   rv = me.process_options()
   if rv != None: return rv

   rv = me.execute()
   if rv != None: return rv

   return me.status

if __name__ == '__main__':
   sys.exit(main())


