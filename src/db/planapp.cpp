/*
 * Plan management application.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "rts2db/appdb.h"
#include "rts2db/plan.h"
#include "rts2db/planset.h"
#include "libnova_cpp.h"
#include "configuration.h"

#include <signal.h>
#include <iostream>

#include <list>

#define OPT_DUMP          OPT_LOCAL + 701
#define OPT_LOAD          OPT_LOCAL + 702
#define OPT_GENERATE      OPT_LOCAL + 703
#define OPT_COPY          OPT_LOCAL + 704
#define OPT_DELETE        OPT_LOCAL + 705
#define OPT_ADD           OPT_LOCAL + 706
#define OPT_DUMP_TARGET   OPT_LOCAL + 707
#define OPT_FIXED_COPY    OPT_LOCAL + 708

class Rts2PlanApp:public rts2db::AppDb
{
    public:
        Rts2PlanApp (int in_argc, char **in_argv);
        virtual ~ Rts2PlanApp (void);

        virtual int doProcessing ();
    protected:
        virtual int processOption (int in_opt);
        virtual int processArgs (const char *arg);

        virtual void usage ();
    private:
        enum { NO_OP, OP_ADD, OP_DUMP, OP_LOAD, OP_GENERATE, OP_COPY, OP_DELETE, OP_DUMP_TARGET, OP_FIXED_COPY } operation;
        int addPlan ();
        void doAddPlan (rts2db::Plan *addedplan);
        int dumpPlan ();
        int loadPlan (const char *fn);
        int generatePlan ();
        int copyPlan (bool st_offsets);
        int deletePlan ();
        int dumpTargetPlan ();

        double parsePlanDate (const char *arg, double base);

        double JD;
        std::vector <const char *> args;
};

Rts2PlanApp::Rts2PlanApp (int in_argc, char **in_argv):rts2db::AppDb (in_argc, in_argv)
{
    JD = NAN;

    operation = NO_OP;

    addOption (OPT_GENERATE, "generate", 0, "generate plan based on targets");
    addOption (OPT_DELETE, "delete", 0, "delete plan with plan ID given as parameter");
    addOption (OPT_DUMP_TARGET, "target", 0, "dump plan for given target");
    addOption (OPT_FIXED_COPY, "fixed-copy", 0, "copy plan with the same times (do not offset ~4min/day)");
    addOption (OPT_COPY, "copy", 0, "copy plan to given night (from night given by -n). Offset times with ~4m/day (to compensate for Earth orbit)");
    addOption (OPT_LOAD, "load", 1, "load plan from file (or standard input, if - is provided)");
    addOption ('n', NULL, 1, "work with this night");
    addOption (OPT_DUMP, "dump", 0, "dump plan to standart output");
    addOption (OPT_ADD, "add", 0, "add plan entry for target (specified as an argument)");
}

Rts2PlanApp::~Rts2PlanApp (void)
{
}

int Rts2PlanApp::addPlan ()
{
    rts2db::Plan *addedplan = NULL;
    for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
    {
        // check if new plan was fully specifed and can be saved
        if (addedplan && !std::isnan (addedplan->getPlanStart ()) && !std::isnan (addedplan->getPlanEnd ()))
        {
            doAddPlan (addedplan);
            delete addedplan;
            addedplan = NULL;
        }

        // create new target if none existed..
        if (!addedplan)
        {
            try
            {
                rts2db::TargetSet tar_set;
                tar_set.load (*iter);
                if (tar_set.empty ())
                {
                    std::cerr << "cannot load target with name/ID" << *iter << std::endl;
                    return -1;
                }
                if (tar_set.size () != 1)
                {
                    std::cerr << "multiple targets matched " << *iter << ", please be more restrictive" << std::endl;
                    return -1;
                }
                addedplan = new rts2db::Plan ();
                addedplan->setTargetId (tar_set.begin ()->second->getTargetID ());
            }
            catch (rts2core::Error &er)
            {
                std::cerr << er << std::endl;
                return -1;
            }
        }
        else if (std::isnan (addedplan->getPlanStart ()))
        {
            addedplan->setPlanStart (parsePlanDate (*iter, getNow ()));
        }
        else
        {
            addedplan->setPlanEnd (parsePlanDate (*iter, addedplan->getPlanStart ()));
        }
    }

    if (addedplan && !std::isnan (addedplan->getPlanStart ()))
    {
        doAddPlan (addedplan);
        delete addedplan;
        return 0;
    }
    delete addedplan;
    return -1;
}

void Rts2PlanApp::doAddPlan (rts2db::Plan *addedplan)
{
    int ret = addedplan->save ();
    if (ret)
    {
        std::ostringstream os;
        os << "cannot save plan " << addedplan->getTarget ()->getTargetName () << " (#" << addedplan->getTargetId () << ")";
        throw rts2core::Error (os.str ());
    }
    std::cout << *addedplan << std::endl;
}


int Rts2PlanApp::dumpPlan ()
{
    rts2db::PlanSetNight plan_set (JD);
    plan_set.load ();
    std::cout << "Plan entries from " << Timestamp (plan_set.getFrom ()) << " to " << Timestamp (plan_set.getTo ()) << std::endl
        << plan_set << std::endl;
    return 0;
}

int Rts2PlanApp::loadPlan (const char *fn)
{
    std::istream *is;
    bool file = false;
    if (strcmp (fn, "-"))
    {
        is = new std::ifstream ();
        ((std::ifstream *) is)->open (fn);
        file = true;
    }
    else
    {
        is = &std::cin;
    }
    rts2db::PlanSet ps;
    try
    {
        *is >> ps;
    }
    catch (rts2core::Error &er)
    {
        std::cerr << "canot load plan file: " << er << std::endl
            << "aborting" << std::endl;
        if (file)
            delete is;
        return -1;
    }
    if (is->fail () && !is->eof ())
    {
        if (file)
        {
            std::cerr << "failed to read " << fn << std::endl;
            delete is;
        }
        else
        {
            std::cerr << "failed to read standard input" << std::endl;
        }
        return -1;
    }
    for (rts2db::PlanSet::iterator iter = ps.begin (); iter != ps.end (); iter++)
    {
        iter->save ();
        iter->load ();
        std::cout << *iter;
    }
    if (file)
        delete is;
    return 0;
}

int Rts2PlanApp::generatePlan ()
{
    return -1;
}

int Rts2PlanApp::copyPlan (bool st_offsets)
{
    if (std::isnan (JD))
    {
        std::cerr << "you must specify source night with -n" << std::endl;
        return -1;
    }

    if (args.empty ())
    {
        std::cerr << "you must provide at least one argument for target night" << std::endl;
        return -1;
    }

    // load source plans..
    rts2db::PlanSetNight plan_set (JD);
    plan_set.load ();

    if (plan_set.empty ())
    {
        std::cerr << "Empty set for dates from " << Timestamp (plan_set.getFrom ()) << " to " << Timestamp (plan_set.getTo ()) << std::endl;
        return -1;
    }

    for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
    {
         double jd2;

         int ret = parseDate (*iter, jd2);
         if (ret)
         {
             std::cerr << "cannot parse date " << *iter << std::endl;
             return -1;
         }

         double d = (jd2 - JD) * 86400;
         if (st_offsets)
         {
            d += 3600 * (ln_get_mean_sidereal_time (JD) - ln_get_mean_sidereal_time (jd2));
         }

         for (rts2db::PlanSet::iterator pi = plan_set.begin (); pi != plan_set.end (); pi++)
         {
            rts2db::Plan np;
            np.setTargetId (pi->getTargetId ());
            np.setPlanStart (pi->getPlanStart () + d);
            if (!std::isnan (pi->getPlanEnd ()))
                np.setPlanEnd (pi->getPlanEnd () + d);
            ret = np.save ();
            if (ret)
            {
                std::cerr << "error saving " << std::endl << (&np) << std::endl << ", exiting" << std::endl;
                return -1;
            }
         }
         std::cout << "copied " << plan_set.size () << " plan entries to " << *iter << std::endl;
    }
    return 0;
}

int Rts2PlanApp::deletePlan ()
{
    // delete night..
    if (!std::isnan (JD))
    {
        rts2db::PlanSetNight pn (JD);
        pn.load ();
        if (pn.empty ())
        {
            std::cerr << "Plan from " << Timestamp (pn.getFrom ()) << " to " << Timestamp (pn.getTo ()) << " is empty" << std::endl;
        }
        else
        {
            std::cout << "Deleting " << pn.size () << " entries from " << Timestamp (pn.getFrom ()) << " to " << Timestamp (pn.getTo ()) << "." << std::endl;
            for (rts2db::PlanSetNight::iterator iter = pn.begin (); iter != pn.end (); iter++)
            {
                if (iter->del ())
                {
                    std::cerr << "cannot delete plan entry " << (*iter) << std::endl;
                    return -1;
                }
                else
                {
                    std::cout << "deleted entry " << *iter << std::endl;
                }
            }
        }
    }
    for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
    {
        int id = atoi (*iter);
        rts2db::Plan p (id);
        if (p.del ())
        {
            std::cerr << "cannot delete plan with ID " << (*iter) << std::endl;
            return -1;
        }
        std::cout << "deleted plan with id " << id << std::endl;
    }
    return 0;
}

int Rts2PlanApp::dumpTargetPlan ()
{
    for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
    {
        rts2db::TargetSet ts;
        ts.load (*iter);
        for (rts2db::TargetSet::iterator tsi = ts.begin (); tsi != ts.end (); tsi++)
        {
            rts2db::PlanSetTarget ps (tsi->second->getTargetID ());
            ps.load ();
            tsi->second->printShortInfo (std::cout);
            std::cout << std::endl << ps << std::endl;
        }
    }
    return 0;
}

double Rts2PlanApp::parsePlanDate (const char *arg, double base)
{
    if (arg[0] == '+')
        return base + atof (arg);
    double d;
    parseDate (arg, d);
    time_t t;
    ln_get_timet_from_julian (d, &t);
    return t;
}

int Rts2PlanApp::processOption (int in_opt)
{
    int ret;
    switch (in_opt)
    {
        case OPT_ADD:
            if (operation != NO_OP)
                return -1;
            operation = OP_ADD;
            break;
        case 'n':
            if (!std::isnan (JD))
            {
                std::cerr << "you can specify night (-n parameter) only once" << std::endl;
                return -1;
            }
            ret = parseDate (optarg, JD);
            if (ret)
                return ret;
            break;
        case OPT_DUMP:
            if (operation != NO_OP)
                return -1;
            operation = OP_DUMP;
            break;
        case OPT_LOAD:
            if (operation != NO_OP)
                return -1;
            operation = OP_LOAD;
            args.push_back (optarg);
            break;
        case OPT_GENERATE:
            if (operation != NO_OP)
                return -1;
            operation = OP_GENERATE;
            break;
        case OPT_COPY:
            if (operation != NO_OP)
                return -1;
            operation = OP_COPY;
            break;
        case OPT_FIXED_COPY:
            if (operation != NO_OP)
                return -1;
            operation = OP_FIXED_COPY;
            break;
        case OPT_DELETE:
            if (operation != NO_OP)
                return -1;
            operation = OP_DELETE;
            break;
        case OPT_DUMP_TARGET:
            if (operation != NO_OP)
                return -1;
            operation = OP_DUMP_TARGET;
            break;
        default:
            return rts2db::AppDb::processOption (in_opt);
    }
    return 0;
}

int Rts2PlanApp::processArgs (const char *arg)
{
    if (operation != OP_ADD && operation != OP_DUMP_TARGET && operation != OP_COPY && operation != OP_DELETE && operation != OP_FIXED_COPY)
        return rts2db::AppDb::processArgs (arg);
    args.push_back (arg);
    return 0;
}

void Rts2PlanApp::usage ()
{
    std::cout << "To add new entry for target 1001 to plan schedule, with observations starting on 2011-01-24 20:00;00 UT, and running for 5 minutes" << std::endl
        << "  " << getAppName () << " --add 1001 2011-01-24T20:00:00 +300" << std::endl
        << std::endl;
}

int Rts2PlanApp::doProcessing ()
{
    switch (operation)
    {
        case NO_OP:
            std::cout << "Please select mode of operation.." << std::endl;
            // interactive..
            return 0;
        case OP_ADD:
            return addPlan ();
        case OP_DUMP:
            return dumpPlan ();
        case OP_LOAD:
            return loadPlan (args.front ());
        case OP_GENERATE:
            return generatePlan ();
        case OP_COPY:
            return copyPlan (true);
        case OP_FIXED_COPY:
            return copyPlan (false);
        case OP_DELETE:
            return deletePlan ();
        case OP_DUMP_TARGET:
            return dumpTargetPlan ();
    }
    return -1;
}

int main (int argc, char **argv)
{
    Rts2PlanApp app = Rts2PlanApp (argc, argv);
    return app.run ();
}
