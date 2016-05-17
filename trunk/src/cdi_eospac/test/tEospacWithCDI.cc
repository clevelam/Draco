//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   cdi_eospac/test/tEospacWithCDI.cc
 * \author Kelly Thompson
 * \date   Thu Apr 19 11:00:24 2001
 * \brief  Implementation file for tEospacWithCDI
 * \note   Copyright (C) 2016 Los Alamos National Security, LLC.
 *         All rights reserved.
 */
//---------------------------------------------------------------------------//
// $Id$
//---------------------------------------------------------------------------//

#include "cdi_eospac/Eospac.hh"
#include "cdi_eospac/SesameTables.hh"
#include "cdi_eospac/EospacException.hh"
#include "ds++/ScalarUnitTest.hh"
#include "ds++/Release.hh"
#include "ds++/Soft_Equivalence.hh"
#include "ds++/SP.hh"
#include "cdi/CDI.hh"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;

namespace rtt_cdi_eospac_test
{
using rtt_dsxx::SP;
using rtt_dsxx::soft_equiv;
   
//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//
/*!
 * \brief Tests the Eospac access routines via the CDI interface.
 *
 * On HPC, the eospac library is located at /usr/projects/data/eos/eospac/...
 */
void eospac_with_cdi_test( rtt_dsxx::UnitTest & ut )
{
    
    cout << endl
         << "Test of C++ code calling EOSPAC routines via the CDI interface." 
         << endl << endl;
    
    // ---------------------------- //
    // Create a SesameTables object //
    // ---------------------------- //
    
    // The user must create a SesameTables object that links material ID
    // numbers to EOSPAC data types (each SesameTables object only contains
    // lookups for one material).  If the user needs heat capacity values for
    // Al then he/she must create a SesameTables object for Aluminum and then
    // assign an aluminum material ID (e.g. 3717) to the enelc EOSPAC data
    // type.  See the tests below for more details.
    
    // Set the material identifier This one is for Aluminum (03717) Category-1
    // data (0) + Mat# 371 (Al) + Version # 7
    
    // See http://xweb.lanl.gov/projects/data/ for material ID information.
    
    // This matID for Al has lookup tables for prtot, entot, tptot, tntot,
    // pntot, eptot, prelc, enelc, tpelc, tnelc pnelc, epelc, prcld, and encld
    // (see SesameTables.hh for an explanantion of these keywords).  I need
    // the table that contains enion lookups so that I can query for Cve()
    // values.

    // Sesame Number 3717 provides data tables: 101 102 201 301 303 304 305 306 401    
    int const Al3717 = 3717;
    
    // I also want to lookup the mean ion charge (EOS_Zfc_DT) which is found
    // in sesame table 601.  Add sesame number 23714 which provides data
    // tables: 101 102 201 601 602 603 604
    int const Al23714 = 23714;
    
    // Create a SesameTables object for Aluminum.
    rtt_cdi_eospac::SesameTables AlSt;
    
    // Assign matID Al3717 to enion lookups (used for Cvi) for AlSt.  We can
    // also assign these tables when the Eospac object is created (see example
    // below).
    
    // Also assign matID Al23714 for temperature-based electron thermal
    // conductivity (tconde).
    AlSt.Uic_DT( Al3717 ).Ktc_DT( Al23714 );

    // Verify that the assignments were made correctly.
    
    // Cvi (returnType=EOS_Uic_DT (=ES4enion)) should point to matID 3717.
    // The user should never need to access this function.  However Eospac.cc
    // does and we need to test this funcitonality.
    
    if ( AlSt.matID( EOS_Uic_DT ) != 3717 )
        FAILMSG("AlSt.matID(EOS_Uic_DT) points to the wrong matID.");
    
    // The temperature-based electorn thermal conductivity
    // (returnType=27=EOS_Ktc_DT) should point to matID 23714.  The user should
    // never need to access this function.  However Eospac.cc does and we need
    // to test this funcitonality.
    
    if ( AlSt.matID( EOS_Ktc_DT ) != 23714 )
        FAILMSG("AlSt.matID(27) points to the wrong matID.");
    
    // ----------------------- //
    // Create an Eospac object //
    // ----------------------- //

    // An Eospac object allows the user to access EoS information about a
    // material that has been constructed in a SesameTable object.  The
    // constructor for Eospac takes one argument: a SesameTables object.
    
    rtt_dsxx::SP< const rtt_cdi::EoS > spEospac;
    
    // Try to instantiate the new Eospac object.  Simultaneously, we are
    // assigned material IDs to more SesameTable values.
    
    if ( 
	spEospac.reset(new const rtt_cdi_eospac::Eospac( 
                           AlSt.Ue_DT( Al3717 ).Zfc_DT( Al23714 ) )), spEospac )

	PASSMSG("SP to new Eospac object created.");
    else
    {
	FAILMSG("Unable to create SP to new Eospac object.");

        // if construction fails, there is no reason to continue testing...
        return;
    }

    // CAUTION!!!  CAUTION!!!  CAUTION!!!  CAUTION!!!  CAUTION!!!  CAUTION!!!
    
    // Adding this block breaks Eospac as currently implemented.  Effectively,
    // the destructor for spEospacAlt destroys all of the table handles for
    // libeospac.  This is a flaw in libeospac, but we may be forced to manage
    // it in cdi_eospac by using a Singleton Eospac object that uses reference
    // counting for each material+data tuple and calls eos_DestroyTables()
    // instead of eos_DestroyAll().  A redesign is also required to make this
    // package thread safe!
    
    // {   // Test alternate ctor method:
        
    //     // Alternatively, we can avoid carrying around the AlSt object.  We
    //     // can, instead, create a temporary version that is only used here in
    //     // the constructor of Eospac().

    //     rtt_dsxx::SP< rtt_cdi_eospac::Eospac const > spEospacAlt;
    //     spEospacAlt = new rtt_cdi_eospac::Eospac(
    //         rtt_cdi_eospac::SesameTables().Ue_DT( Al3717 ).Zfc_DT( Al23714
    //             ).Uic_DT( Al3717 ).Ktc_DT( Al23714 ) );
        
    //     if ( spEospacAlt )
    //         PASSMSG("SP to new Eospac object created (Alternate ctor).");
    //     else
    //         FAILMSG("Unable to create SP to new Eospac object (Alternate ctor).");
    // }
    
    // ------------------- //
    // Create a CDI object //
    // ------------------- //
    
    rtt_dsxx::SP< rtt_cdi::CDI > spCdiEos;
    if ( spCdiEos.reset(new rtt_cdi::CDI()), spCdiEos )
	PASSMSG("SP to CDI object created successfully.");
    else
	FAILMSG("Failed to create SP to CDI object.");

    // -------------------- //
    // Assign Eos to CDI    //
    // -------------------- //

    spCdiEos->setEoS( spEospac );
    if ( spCdiEos->isEoSSet() )
        PASSMSG("Eospac object assigned to CDI successfully.");
    else
        FAILMSG("Failed to assign Eospac object to CDI object.");
    
    // --------------------------- //
    // Test scalar access routines //
    // --------------------------- //
    
    double const K2keV = 1.0/1.1604412E+7; // keV/Kelvin
    
    // All of these tests request an EoS value given a single
    // temperature and a single density.
    
    // Retrieve an Electron internal energy value;
    
    double density     = 1.0;  // g/cm^3
    double temperature = 5800; // K
    temperature *= K2keV;      // convert temps to keV.
    
    double refValue =  1.0507392783; // kJ/g
    
    double specificElectronInternalEnergy =
        spCdiEos->eos()->getSpecificElectronInternalEnergy(
            temperature, density );
    double const tol(1.0e-10);
    
    if ( soft_equiv( specificElectronInternalEnergy, refValue, tol ) )
        PASSMSG("getSpecificElectronInternalEnergy() test passed.");
    else
        FAILMSG("getSpecificElectronInternalEnergy() test failed.");
    
    // Retrieve an electron heat capacity (= dE/dT)	    
    
    refValue =  4101.9991645 ; // kJ/g/keV
    
    double heatCapacity =
        spCdiEos->eos()->getElectronHeatCapacity( 
            temperature, density );
    
    if ( soft_equiv(  heatCapacity, refValue, tol ) )
        PASSMSG("getElectronHeatCapacity() test passed.");
    else
        FAILMSG("getElectronHeatCapacity() test failed.");
    
    // Retrive an Ion Internal Energy
    
    refValue = 5.23391652028; // kJ/g
    
    double specificIonInternalEnergy = 
        spCdiEos->eos()->getSpecificIonInternalEnergy( 
            temperature, density );
    
    if ( soft_equiv( specificIonInternalEnergy, refValue, tol ) )
        PASSMSG("getSpecificIonInternalEnergy() test passed.");
    else
        FAILMSG("getSpecificIonInternalEnergy() test failed.");
    
    // Retrieve an ion based heat capacity
    
    refValue =  6748.7474603; // kJ/g/keV
    
    heatCapacity =
        spCdiEos->eos()->getIonHeatCapacity( temperature, density );
    
    if ( soft_equiv( heatCapacity, refValue, tol ) )
        PASSMSG("getIonHeatCapacity() test passed.");
    else
        FAILMSG("getIonHeatCapacity() test failed.");
    
    // Retrieve the number of free electrons per ion
    
    refValue = 12.8992087458; // electrons per ion
    
    double nfree =
        spCdiEos->eos()->getNumFreeElectronsPerIon( 
            temperature, density );
    
    if ( soft_equiv( nfree, refValue, tol ) )
        PASSMSG("getNumFreeElectronsPerIon() test passed.");
    else
        FAILMSG("getNumFreeElectronsPerIon() test failed.");
    
    // Retrieve the electron based thermal conductivity
    
    refValue =  1.38901721467e+29; // 1/s/cm
    
    double chie = 
        spCdiEos->eos()->getElectronThermalConductivity(
            temperature, density );
    
    if ( soft_equiv( chie, refValue, tol ) )
        PASSMSG("getElectronThermalConductivity() test passed.");
    else
        FAILMSG("getElectronThermalConductivity() test failed.");
    
    // --------------------------- //
    // Test vector access routines //
    // --------------------------- //
    
    // Set up simple temp and density vectors.  vtemp(i) will
    // always be associated with vdensities(i).  In this case
    // both tuples have identical data so that the returned
    // results will also be identical.
    
    vector< double > vtemps(2);
    vector< double > vdensities(2);
    
    vtemps[0] = temperature;
    vtemps[1] = temperature;
    vdensities[0] = density;
    vdensities[1] = density;
    
    // Retrieve electron based heat capacities for each set of 
    // (density, temperature) values.
    
    vector< double > vcve(2);
    vcve = spCdiEos->eos()->getElectronHeatCapacity( 
        vtemps, vdensities );
    
    // Since the i=0 and i=1 tuples of density and temperature 
    // are identical the two returned heat capacities should
    // also match.
    
    if ( soft_equiv( vcve[0], vcve[1], tol ) )
    {
        ostringstream msg;
        msg << "getElectronHeatCapacity() test passed for " 
            << "vector state values.";
        PASSMSG( msg.str() );
    }
    else
    {
        ostringstream msg;
        msg << "getElectronHeatCapacity() test failed for "
            << "vector state values.";
        FAILMSG( msg.str() );
    }
    
    // This result should also match the scalar value
    // calculated above.
    
    heatCapacity =
        spCdiEos->eos()->getElectronHeatCapacity( 
            temperature, density );
    
    if ( soft_equiv( vcve[0], heatCapacity, tol ) )
    {
        ostringstream msg;
        msg << "getElectronHeatCapacity() test passed for " 
            << "vector state values.";
        PASSMSG( msg.str() );
    }
    else
    {
        ostringstream msg;
        msg << "getElectronHeatCapacity() test failed for "
            << "vector state values.";
        FAILMSG( msg.str() );
    }
    
} // end of tests

} // end of namespace rtt_cdi_eospac_test
    
//---------------------------------------------------------------------------//

int main(int argc, char *argv[])
{
    rtt_dsxx::ScalarUnitTest ut( argc, argv, rtt_dsxx::release );
    try { rtt_cdi_eospac_test::eospac_with_cdi_test(ut); }
    UT_EPILOG(ut);
}   

//---------------------------------------------------------------------------//
// end of tEospacwithCDI.cc
//---------------------------------------------------------------------------//