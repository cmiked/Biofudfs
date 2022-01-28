#include "udf.h"
FILE *fid;

/* Ta define profile orizoun tin oriaki synthiki se kathe astheni, 
kai grafoun se ena arxeio ta zoneid's twn epifaneiwn pou tha prepei na allazoyn se kathe iteration


Ta define adjust diavazoun se kathe iteration ta zoneid's kai 
epivaloun tis katallilew oriakes synthikew gia thn piesi/*/

// inlet_pressure_pat_a: name of boundary condition
// t: thread (patient inlet)
// i: number of flow variable, gets defined from the boundary condition dialog box
DEFINE_PROFILE(outlet_pressure_pat_a,t,i)
{
  #if !RP_HOST
  face_t f;
  #endif /* !RP_HOST */
  
  int zoneid = THREAD_ID(t);
  real volume, old_flux, Ri, Rzi, newpress;

  /* Write the property and thread id values for the flux condition to file */
  fid = fopen("pat_a_inlet_id", "w");
  fprintf(fid, "%d, %d\n", zoneid, i);
  fclose(fid);
  //-----READING PATIENT VOLUME AND OLD FLUX----//
  fid = fopen("pat_a_volume.dat", "r");
  fscanf(fid,"%lf, %lf", &volume, &old_flux);
  fclose(fid);

  //------READING PATIENT A DATA-----------//
  fid = fopen("patient_a.dat", "r");
  fscanf(fid,"%lf, %lf", &Ri, &Rzi);
  fclose(fid);
  //------Done Reading Patirnt a data---------//

   

  #if !RP_HOST
  newpress = volume/35*98.06  + (Ri + Rzi) * 98.06 * 1.e3* (old_flux )/2.0;
  /* Set profile values based on old flux and volume */
  begin_f_loop(f,t)
  {
      F_PROFILE(f,t,i) = newpress;
  }
  end_f_loop(f,t)
  #endif /* !RP_HOST */
}




DEFINE_PROFILE(outlet_pressure_pat_b,t,i)
{
  #if !RP_HOST
  face_t f;
  #endif /* !RP_HOST */
  int zoneid = THREAD_ID(t);
  real volume, old_flux, Ri, Rzi, newpress;

  /* Write the property and thread id values for the flux condition to file */
  fid = fopen("pat_b_inlet_id", "w");
  fprintf(fid, "%d, %d\n", zoneid, i);
  fclose(fid);
  //-----READING PATIENT VOLUME AND OLD FLUX----//
  fid = fopen("pat_b_volume.dat", "r");
  fscanf(fid,"%lf, %lf", &volume, &old_flux);
  fclose(fid);

  //------READING PATIENT A DATA-----------//
  fid = fopen("patient_b.dat", "r");
  fscanf(fid,"%lf, %lf", &Ri, &Rzi);
  fclose(fid);
  //------Done Reading Patirnt a data---------//

   

  #if !RP_HOST
  newpress = volume/35*98.06  + (Ri + Rzi) * 98.06 * 1.e3* (old_flux )/2.0;
  /* Set profile values based on old flux and volume */
  begin_f_loop(f,t)
  {
      F_PROFILE(f,t,i) = newpress;
  }
  end_f_loop(f,t)
  #endif /* !RP_HOST */
}




//Define adjust for patient a
DEFINE_ADJUST(face_pressure_set_pat_a,domain)
{
  int surface_thread_id, iprop ;
  real total_flux=0.0; //initializing total flux

  
  fid = fopen("pat_a_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d, %d", &surface_thread_id, &iprop);
  fclose(fid);

  #if !RP_HOST
    Thread* thread; /* these variables are only defined on "calculating" processes (Nodes & serial)*/
    face_t face;
  #endif /* !RP_HOST */

  host_to_node_int_1(surface_thread_id); /* Does nothing in SERIAL */

  /* #if RP_NODE
  Message("\n Node %d is calculating on thread # %d \n",myid,surface_thread_id);
  #endif  RP_NODE */ 

  #if !RP_HOST /* SERIAL or NODE */
    thread = Lookup_Thread(domain,surface_thread_id);

    begin_f_loop(face,thread)
      # if RP_NODE
        if (I_AM_NODE_SAME_P(F_PART(face,thread))) /* Check to see if face is allocated to this partition (Actually C0 of face) */
      # endif
      {
      total_flux += F_FLUX(face,thread);
      }
    end_f_loop(face,thread)


    //summing flux across all nodes
    # if RP_NODE
      total_flux = PRF_GRSUM1(total_flux);
    #endif /* RP_NODE */

  #endif /* !RP_HOST */

  node_to_host_real_1(total_flux); /* Does nothing in SERIAL */
  Message0("Total flux of patient A is : %f \n", total_flux);

}

//Define adjust for patient B
DEFINE_ADJUST(face_pressure_set_pat_b,domain)
{
  int surface_thread_id, iprop ;
  real total_flux=0.0; //initializing total flux

  
  fid = fopen("pat_b_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d, %d", &surface_thread_id, &iprop);
  fclose(fid);

  #if !RP_HOST
    Thread* thread; /* these variables are only defined on "calculating" processes (Nodes & serial)*/
    face_t face;
  #endif /* !RP_HOST */

  host_to_node_int_1(surface_thread_id); /* Does nothing in SERIAL */

  /* #if RP_NODE
  Message("\n Node %d is calculating on thread # %d \n",myid,surface_thread_id);
  #endif  RP_NODE */ 

  #if !RP_HOST /* SERIAL or NODE */
    thread = Lookup_Thread(domain,surface_thread_id);

    begin_f_loop(face,thread)
      # if RP_NODE
        if (I_AM_NODE_SAME_P(F_PART(face,thread))) /* Check to see if face is allocated to this partition (Actually C0 of face) */
      # endif
      {
      total_flux += F_FLUX(face,thread);
      }
    end_f_loop(face,thread)


    //summing flux across all nodes
    # if RP_NODE
      total_flux = PRF_GRSUM1(total_flux);
    #endif /* RP_NODE */

  #endif /* !RP_HOST */

  node_to_host_real_1(total_flux); /* Does nothing in SERIAL */
  Message0("Total flux of patient B is : %f \n", total_flux);

}




//changing patient A volume
DEFINE_EXECUTE_AT_END(renew_vol_pat_a)
{

  Domain *domain;
  real total_flux, volume, old_flux, timestep;
  int surface_thread_id, iprop;
  fid = fopen("pat_a_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d, %d", &surface_thread_id, &iprop);
  fclose(fid);
  domain = Get_Domain(1);
  
  #if !RP_HOST
    Thread* thread; /* these variables are only defined on "calculating" processes (Nodes & serial)*/
    face_t face;
  #endif /* !RP_HOST */


  #if !RP_HOST /* SERIAL or NODE */
    thread = Lookup_Thread(domain,surface_thread_id);

    begin_f_loop(face,thread)
      # if RP_NODE
        if (I_AM_NODE_SAME_P(F_PART(face,thread))) /* Check to see if face is allocated to this partition (Actually C0 of face) */
      # endif
      {
      total_flux += F_FLUX(face,thread);
      }
    end_f_loop(face,thread)


    //summing flux across all nodes
    # if RP_NODE
      total_flux = PRF_GRSUM1(total_flux);
    #endif /* RP_NODE */

  #endif /* !RP_HOST */
  node_to_host_real_1(total_flux); /* Does nothing in SERIAL */


  fid = fopen("pat_a_volume.dat", "r");
  fscanf(fid,"%lf, %lf", &volume, &old_flux);
  fclose(fid);
  timestep = CURRENT_TIMESTEP;
  // no density volume in mm^3
  volume = volume + 1.e6*(total_flux + old_flux)/2*timestep;
  Message0("Patient A volume: %lf  \n",volume);
  fid = fopen("pat_a_volume.dat", "w");
  fprintf(fid, "%lf, %lf\n", volume, total_flux);
  fclose(fid);

} 


//changing patient B volume
DEFINE_EXECUTE_AT_END(renew_vol_pat_b)
{

  Domain *domain;
  real total_flux, volume, old_flux, timestep;
  int surface_thread_id, iprop;
  fid = fopen("pat_b_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d, %d", &surface_thread_id, &iprop);
  fclose(fid);
  domain = Get_Domain(1);
  
  #if !RP_HOST
    Thread* thread; /* these variables are only defined on "calculating" processes (Nodes & serial)*/
    face_t face;
  #endif /* !RP_HOST */


  #if !RP_HOST /* SERIAL or NODE */
    thread = Lookup_Thread(domain,surface_thread_id);

    begin_f_loop(face,thread)
      # if RP_NODE
        if (I_AM_NODE_SAME_P(F_PART(face,thread))) /* Check to see if face is allocated to this partition (Actually C0 of face) */
      # endif
      {
      total_flux += F_FLUX(face,thread);
      }
    end_f_loop(face,thread)


    //summing flux across all nodes
    # if RP_NODE
      total_flux = PRF_GRSUM1(total_flux);
    #endif /* RP_NODE */

  #endif /* !RP_HOST */
  node_to_host_real_1(total_flux); /* Does nothing in SERIAL */


  fid = fopen("pat_b_volume.dat", "r");
  fscanf(fid,"%lf, %lf", &volume, &old_flux);
  fclose(fid);
  timestep = CURRENT_TIMESTEP;
  // no density volume in mm^3
  volume = volume + 1.e6*(total_flux + old_flux)/2*timestep;
  Message0("Patient B volume: %lf  \n",volume);
  fid = fopen("pat_b_volume.dat", "w");
  fprintf(fid, "%lf, %lf\n", volume, total_flux);
  fclose(fid);

} 

DEFINE_PROFILE(inlet_varying_flux,thread,i)
{
 face_t face;
 #if !RP_HOST /* SERIAL or NODE */
   

    begin_f_loop(face,thread)
      # if RP_NODE
        if (I_AM_NODE_SAME_P(F_PART(face,thread))) /* Check to see if face is allocated to this partition (Actually C0 of face) */
      # endif
      {
        if(CURRENT_TIME<=2) F_PROFILE(face,thread,i) = 2*5.21e-4 * ( 1 - pow(cos(sin(3.1415*CURRENT_TIME/2.0)),400));
        else F_PROFILE(face,thread,i) =0.0;
      }
    end_f_loop(face,thread)
    #endif /* !RP_HOST */
}