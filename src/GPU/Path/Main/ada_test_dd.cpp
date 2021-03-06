#include "ada_test_dd.h"
#include <qd/qd_real.h>

void var_name ( char* x_string, int x_string_len, string*& x_names, int& dim )
{
   dim = 1;
   for(int pos=0; pos<x_string_len; pos++)
      if(x_string[pos]==' ') dim++;
   x_names = new string[dim];

   int begin=0;
   string mystring = x_string;
   int var_idx = 0;
   for(int pos=0; pos<x_string_len; pos++)
   {
      if(x_string[pos]==' ')
      {
         x_names[var_idx] = mystring.substr(begin, pos-begin);
         begin = pos+1;
         var_idx++;
      }
   }
   x_names[var_idx] = mystring.substr(begin, x_string_len-begin);
}

void ada_read_sys ( int verbose, PolySys& sys )
{
   int fail,nbsym;

   fail = syscon_number_of_symbols(&nbsym);

   if(verbose > 0)
   {
      std::cout << "the system is .." << std::endl;
      fail = syscon_write_dobldobl_system();
      std::cout << "the number of symbols : " << nbsym << std::endl;
   }
   int s_dim = 80*nbsym;
   char *s = (char*) calloc(s_dim,sizeof(char));
   fail = syscon_string_of_symbols(&s_dim, s);

   string* x_names;
   int dim = 0;
   var_name(s, s_dim, x_names, dim);
   int i = 1;
   if(verbose > 0) std::cout << "dim = " << dim << std::endl;

   double c[4]; /* two consecutive double doubles are real and imag parts */
   int d[dim];

   int n_eq = 0;
   fail = syscon_number_of_dobldobl_polynomials(&n_eq);

   sys.n_eq = n_eq;
   sys.dim  = dim;
   sys.eq_space = new PolyEq[n_eq];
   sys.pos_var = x_names;

   PolyEq* tmp_eq = sys.eq_space;

   for(int i=1; i<n_eq+1; i++)
   {
      int nt;
      fail = syscon_number_of_dobldobl_terms(i,&nt);
      if(verbose > 0)
         std::cout << " #terms in polynomial " << i << " : " << nt << std::endl;
      tmp_eq->n_mon = nt;
      tmp_eq->dim = dim;
      for(int j=1; j<=nt; j++)
      {
         fail = syscon_retrieve_dobldobl_term(i,j,dim,d,c);
         if(verbose > 0)
         {
            std::cout << c[0] << " " << c[2] << std::endl;
            for (int k=0; k<dim; k++) std::cout << " " << d[k];
            std::cout << std::endl;
         }
         bool constant_term = true;
         for(int k=0; k<dim; k++)
            if(d[k]!=0) constant_term = false;

         if(constant_term==true)
         {
            tmp_eq->n_mon--;
            tmp_eq->constant += CT(c[0],c[2]);
         }
         else
         {
            T1 cffs[2];
            T1 realpcff;
            T1 imagpcff;
            realpcff.x[0] = c[0];
            realpcff.x[1] = c[1];
            imagpcff.x[0] = c[2];
            imagpcff.x[1] = c[3];
            cffs[0] = realpcff;
            cffs[1] = imagpcff;
            PolyMon* a = new PolyMon(dim,d,cffs);
            tmp_eq->mon.push_back(a);
         }
      }
      if(verbose > 0) tmp_eq->print(x_names);
      sys.eq.push_back(tmp_eq);
      tmp_eq++;
   }
   if(verbose > 0)
   {
      sys.print();
      std::cout << "End" << std::endl;
   }
}

void ada_read_sols ( PolySys& start_sys, PolySolSet& sols )
{
   int fail, len;

   fail = solcon_number_of_dobldobl_solutions(&len);
   // printf("Number of start solutions : %d\n",len);
   int dim=start_sys.dim;
   sols.dim = dim;
   double sol[4*dim+10];

   for(int sol_idx=1; sol_idx<len+1; sol_idx++)
   {
      int mm,k,pos;
      // T1 dd_sol[2*dim];
      T1* dd_sol;
      T1 t_real,t_imag,err,rco,res;

      dd_sol = (T1*)calloc(2*dim, sizeof(T1));

      solcon_retrieve_next_dobldobl_solution(dim,&k,&mm,sol);
      //solcon_retrieve_solution(dim,sol_idx,&mm,sol);
      /*std::cout << sol[0] << " " << sol[1] << std::endl;
      for(int var_idx=0; var_idx<4; var_idx++)
         std::cout << sol[2+2*var_idx] << " " << sol[2+2*var_idx] << std::endl;
      std::cout << sol[2+2*dim] 
                << " " << sol[3+2*dim]
                << " " << sol[4+2*dim] << std::endl;*/
      t_real.x[0] = sol[0];
      t_real.x[1] = sol[1];
      t_imag.x[0] = sol[2];
      t_imag.x[1] = sol[3];
      pos = 4;
      for(int dd_sol_idx=0; dd_sol_idx<2*dim; dd_sol_idx++)
      {
         T1 sol_real,sol_imag;
         sol_real.x[0] = sol[pos++];
         sol_real.x[1] = sol[pos++];
         sol_imag.x[0] = sol[pos++];
         sol_imag.x[1] = sol[pos++];
         dd_sol[dd_sol_idx++] = sol_real;
         dd_sol[dd_sol_idx] = sol_imag;
      }
      err.x[0] = sol[4*dim+4];
      err.x[1] = sol[4*dim+5];
      rco.x[0] = sol[4*dim+6];
      rco.x[1] = sol[4*dim+7];
      res.x[0] = sol[4*dim+8];
      res.x[1] = sol[4*dim+9];
      PolySol* tmp_sol = new PolySol(dim,t_real,t_imag,dd_sol,err,rco,res);
      //tmp_sol->print_info(start_sys.pos_var);
      sols.add_sol(tmp_sol);
   }

   // std::cout << "sol finished" << std::endl;
}

void ada_write_sols ( PolySolSet& sols )
{
   int fail = solcon_clear_dobldobl_solutions();
   if(fail != 0)
      std::cout << "failed to clear the solutions" << std::endl;
   int dim = sols.dim;
   int nbsols = sols.n_sol;

   for(int sol_idx=0; sol_idx<nbsols; sol_idx++)
   {
      CT* sol = sols.get_sol(sol_idx);
      double csol[4*dim+10];
      csol[0] = 0.0; csol[1] = 0.0;
      csol[2] = 0.0; csol[3] = 0.0;
      int idx = 4;
      for(int k=0; k<dim; k++)
      {
         csol[idx++] = sol[k].real.x[0];
         csol[idx++] = sol[k].real.x[1];
         csol[idx++] = sol[k].imag.x[0];
         csol[idx++] = sol[k].imag.x[1];
      }
      csol[4*dim+4] = 0.0; csol[4*dim+5] = 0.0;
      csol[4*dim+6] = 0.0; csol[4*dim+7] = 0.0;
      csol[4*dim+8] = 0.0; csol[4*dim+9] = 0.0;
      fail = solcon_append_dobldobl_solution(dim,1,csol);
      if(fail != 0)
         std::cout << "failed to append the solution" << std::endl;
   }
}

void ada_read_homotopy
 ( char* start_file, char* target_file,
   PolySys& start_sys, PolySys& target_sys, PolySolSet& sols )
{
   int fail;

   std::cout << target_file << " " << strlen(target_file) << std::endl;
   std::cout << start_file << " " << strlen(start_file) << std::endl;
   fail = read_dobldobl_target_system_from_file
            (strlen(target_file), target_file);
   ada_read_sys(0,target_sys);

   fail = read_dobldobl_start_system_from_file(strlen(start_file),start_file);
   ada_read_sys(0,start_sys);
   ada_read_sols(start_sys, sols);
}
