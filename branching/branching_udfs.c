#include "udf.h"
FILE *fid;

/* Ta define profile orizoun tin oriaki synthiki se kathe astheni, 
kai grafoun se ena arxeio ta zoneid's twn epifaneiwn pou tha prepei na allazoyn se kathe iteration


Ta define adjust diavazoun se kathe iteration ta zoneid's kai 
epivaloun tis katallilew oriakes synthikew gia thn piesi/*/

// inlet_pressure_pat_a: name of boundary condition
// t: thread (patient inlet)
// i: number of flow variable, gets defined from the boundary condition dialog box


//adjusting deltat
DEFINE_DELTAT(mydeltat,d)
{
   real time_step;
   real flow_time = CURRENT_TIME;
   if (flow_time < 0.03 || (flow_time > 1.9 && flow_time < 4)) { time_step = 0.001;}
   else time_step = 0.01;
   return time_step;
}

//in order to execute define profiles only once per timestep
//Kanonika ta define profile ektelountai se interval iterations opws exei oristei sto gui
//emeis theloyme na ektelountai mia fora sthn arxi kathe timestep
//giafto orizontai oi pseudoxronoi a_time b_time c_time (current time)
//an o a-b_time diaferei apo ton c_time ekteleitai to define_profile kai a-b_time ginetai isos me c_time
//alliws oxi

//o c_time allazei sto define_adjust
real c_time = 0.0;
real a_time = -0.1;
real b_time = -0.1;


/* Reading old flux, volume and adjusting pressure ath the begining of time step*/
DEFINE_PROFILE(outlet_pressure_pat_a,t,i)
{
  if(a_time!=c_time){
  #if !RP_HOST
  face_t f;
  #endif /* !RP_HOST */
  
  int zoneid = THREAD_ID(t);
  real volume, old_flux, Ri, Rzi, newpress, Ci;

  /* Write the property and thread id values for the flux condition to file */
  fid = fopen("pat_a_inlet_id", "w");
  fprintf(fid, "%d %d\n", zoneid, i);
  fclose(fid);
  //-----READING PATIENT VOLUME AND OLD FLUX----//
  fid = fopen("pat_a_volume.dat", "r");
  fscanf(fid,"%lf %lf", &volume, &old_flux);
  fclose(fid);

  //------READING PATIENT A DATA-----------//
  fid = fopen("patient_a.dat", "r");
  fscanf(fid,"%lf %lf %lf", &Ri, &Rzi, &Ci);
  fclose(fid);
  //------Done Reading Patirnt a data---------//


  #if !RP_HOST
  newpress = volume/Ci*98.06  + (Ri + Rzi) * 98.06 * 1.e3* (old_flux );
  /* Set profile values based on old flux and volume */
  begin_f_loop(f,t)
  {
      F_PROFILE(f,t,i) = newpress;
  }
  end_f_loop(f,t)
  #endif /* !RP_HOST */
  a_time = c_time;
  }
}




DEFINE_PROFILE(outlet_pressure_pat_b,t,i)
{
  if(b_time != c_time){
  #if !RP_HOST
  face_t f;
  #endif /* !RP_HOST */
  int zoneid = THREAD_ID(t);
  real volume, old_flux, Ri, Rzi, newpress, Ci;

  /* Write the property and thread id values for the flux condition to file */
  fid = fopen("pat_b_inlet_id", "w");
  fprintf(fid, "%d %d\n", zoneid, i);
  fclose(fid);
  //-----READING PATIENT VOLUME AND OLD FLUX----//
  fid = fopen("pat_b_volume.dat", "r");
  fscanf(fid,"%lf %lf", &volume, &old_flux);
  fclose(fid);

  //------READING PATIENT A DATA-----------//
  fid = fopen("patient_b.dat", "r");
  fscanf(fid,"%lf %lf %lf", &Ri, &Rzi, &Ci);
  fclose(fid);
  //------Done Reading Patirnt a data---------//

  //comment uncomment this section for vane adjustment
  //---------------------ADJUSTING VANE---------------------//
  /*
  float newRzi= 19.0 - 14.0;
  float time = CURRENT_TIME;
  float Q0 = 521.0;
  float e = Q0*0.0;
  float c = (1.0/35.0 - 1.0/51.0)*1.e3;
  float Vtot = 1000.0;
  if((CURRENT_TIME>= 0.04 )&& (CURRENT_TIME <= 2.0)){
    if (time>1.96) time = 1.96;
    newRzi += ((time-1.0)*Q0 + Vtot/2)*c/(e + Q0 * (1 - pow(cos(sin(3.1415 / 2.0 *time )),400.0)));
  }
  

  fid = fopen("patient_b.dat", "w");
  fprintf(fid,"14.0 %f 51.0", newRzi);
  fclose(fid);

  Rzi = newRzi;
  Message0("Rz= %f   %lf \n", newRzi, Rzi );
  */
  
 //-----------------DONE ADJUSTING -----------------------//

  #if !RP_HOST
  newpress = volume/Ci*98.06  + (Ri + Rzi) * 98.06 * 1.e3* (old_flux);
  /* Set profile values based on old flux and volume */
  begin_f_loop(f,t)
  {
      F_PROFILE(f,t,i) = newpress;
  }
  end_f_loop(f,t)
  #endif /* !RP_HOST */


  b_time = c_time;
  }

}




//Define adjust for patient a
//Calculating flox at patient, applying pressure calculated by current flux 
DEFINE_ADJUST(face_pressure_set_pat_a,domain)
{
  c_time = CURRENT_TIME;
  int surface_thread_id, iprop ;
  real total_flux=0.0; //initializing total flux
  real volume, old_flux, Ri, Rzi, Ci, pseudonewpress;

  
  fid = fopen("pat_a_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d %d", &surface_thread_id, &iprop);
  fclose(fid);

  //-----READING PATIENT VOLUME AND OLD FLUX----//
  fid = fopen("pat_a_volume.dat", "r");
  fscanf(fid,"%lf %lf", &volume, &old_flux);
  fclose(fid);

  //------READING PATIENT A DATA-----------//
  fid = fopen("patient_a.dat", "r");
  fscanf(fid,"%lf %lf %lf", &Ri, &Rzi, &Ci);
  fclose(fid);


  #if !RP_HOST
    Thread* thread; /* these variables are only defined on "calculating" processes (Nodes & serial)*/
    face_t face;
  #endif /* !RP_HOST */

  host_to_node_int_1(surface_thread_id); /* Does nothing in SERIAL */

  /* #if RP_NODE
  Message("\n Node %d is calculating on thread # %d \n",myid,surface_thread_id);
  #endif  RP_NODE */ 
//SUMMING FLUX ACROSS ALL FACES OF PATIENT
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
  Message0("Total flux of patient A is : %f Kg/s\n", total_flux);

  #if !RP_HOST /* SERIAL or NODE */
    thread = Lookup_Thread(domain,surface_thread_id);
    if(CURRENT_TIME > 1.96 || CURRENT_TIME < 0.01) {old_flux = total_flux = 0.0; pseudonewpress =0.0;}
    else pseudonewpress = (volume + (old_flux*0.5 + total_flux*0.5) * CURRENT_TIMESTEP)/Ci*98.06  + (Ri + Rzi) * 98.06 * 1.e3* (total_flux*0.7 + old_flux*0.3 );
    //LOOPING THROUGH ALL FACES 
    begin_f_loop(face,thread)
    # if RP_NODE
      if (I_AM_NODE_SAME_P(F_PART(face,thread))) /* Check to see if face is allocated to this partition (Actually C0 of face) */
    # endif
    { 
      //ASSIGNING NEW PRESSURE AT THREAD (WHICH IS THE INLET OF THE RESPECTIVE PATIENT)
      F_PROFILE(face,thread,iprop) = pseudonewpress;
    }
    end_f_loop(face,thread)

  #endif /* !RP_HOST */

}

//Define adjust for patient B
DEFINE_ADJUST(face_pressure_set_pat_b,domain)
{
  int surface_thread_id, iprop ;
  real total_flux=0.0; //initializing total flux
  real volume, old_flux, Ri, Rzi, Ci, pseudonewpress;

  
  fid = fopen("pat_b_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d %d", &surface_thread_id, &iprop);
  fclose(fid);

  //-----READING PATIENT VOLUME AND OLD FLUX----//
  fid = fopen("pat_b_volume.dat", "r");
  fscanf(fid,"%lf %lf", &volume, &old_flux);
  fclose(fid);

  //------READING PATIENT A DATA-----------//
  fid = fopen("patient_b.dat", "r");
  fscanf(fid,"%lf %lf %lf", &Ri, &Rzi, &Ci);
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
  Message0("Total flux of patient B is : %f Kg/s\n", total_flux);

  #if !RP_HOST /* SERIAL or NODE */
    thread = Lookup_Thread(domain,surface_thread_id);
    if(CURRENT_TIME > 1.96 || CURRENT_TIME < 0.01) {old_flux = total_flux = 0.0; pseudonewpress = 0.0;}
    else pseudonewpress = (volume + (old_flux*0.5 + total_flux*0.5) * CURRENT_TIMESTEP)/Ci*98.06  + (Ri + Rzi) * 98.06 * 1.e3* (total_flux*0.7 + old_flux*0.3);
    //LOOPING THROUGH ALL FACES 
    begin_f_loop(face,thread)
    # if RP_NODE
      if (I_AM_NODE_SAME_P(F_PART(face,thread))) /* Check to see if face is allocated to this partition (Actually C0 of face) */
    # endif
    { 
      //ASSIGNING NEW PRESSURE AT THREAD (WHICH IS THE INLET OF THE RESPECTIVE PATIENT)
     
      F_PROFILE(face,thread,iprop) = pseudonewpress;
    }
    end_f_loop(face,thread)

  #endif /* !RP_HOST */

}




//changing patient A volume
DEFINE_EXECUTE_AT_END(renew_vol_pat_a)
{

  Domain *domain;
  real total_flux, volume, old_flux, timestep;
  total_flux = 0.0;
  int surface_thread_id, iprop;
  fid = fopen("pat_a_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d %d", &surface_thread_id, &iprop);
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
  fscanf(fid,"%lf %lf", &volume, &old_flux);
  fclose(fid);
  timestep = CURRENT_TIMESTEP;
  // no density volume in mm^3
  volume = volume + 1.e6*(total_flux + old_flux)/2.0*timestep;
  Message0("Patient A volume: %lf  \n",volume);
  fid = fopen("pat_a_volume.dat", "w");
  fprintf(fid, "%lf %lf\n", volume, total_flux);
  fclose(fid);

} 


//changing patient B volume
DEFINE_EXECUTE_AT_END(renew_vol_pat_b)
{

  Domain *domain;
  real total_flux, volume, old_flux, timestep;
  total_flux = 0.0;
  int surface_thread_id, iprop;
  fid = fopen("pat_b_inlet_id", "r");
  //reading the surface id and the flow variable id
  fscanf(fid,"%d %d", &surface_thread_id, &iprop);
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
  fscanf(fid,"%lf %lf", &volume, &old_flux);
  fclose(fid);
  timestep = CURRENT_TIMESTEP;
  // no density volume in mm^3
  volume = volume + 1.e6*(total_flux + old_flux)/2.0*timestep;
  Message0("Patient B volume: %lf  \n",volume);
  fid = fopen("pat_b_volume.dat", "w");
  fprintf(fid, "%lf %lf\n", volume, total_flux);
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