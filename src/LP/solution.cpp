// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <solution.h>

int64_t GLPKSolution::t_limit = -1;

GLPKSolution::GLPKSolution(const LinearProgram &lp, unsigned int max_var_num,
                           double var_lb, double var_ub, uint dir, uint aim)
    : glpk(glp_create_prob()),
      lp(lp),
      col_num(max_var_num),
      row_num(lp.get_equalities().size() + lp.get_inequalities().size()),
      coeff_num(0),
      is_mip(lp.has_binary_variables() || lp.has_integer_variables()),
      dir(dir),
      aim(aim),
      solved(false) 
  {
     cout<<"hello!"<<endl;
    cout<<"col_num:"<<max_var_num<<"row_num"<<lp.get_equalities().size() + lp.get_inequalities().size()<<"is_mip:";
    if(lp.has_binary_variables())
    cout<< "lp.has_binary_variables()" ;
    if(lp.has_integer_variables())
    cout<< "lp.has_integer_variables()"<<endl;
    cout<<"dir:"<<dir<<"aim"<<aim<<endl;
  if (col_num)
    solve(var_lb, var_ub);
  else
    // Trivial case: no variables.
    // This can happen if a task set does not
    // contain any shared resources.
    solved = true;
}

GLPKSolution::~GLPKSolution() { glp_delete_prob(glpk); }

void GLPKSolution::callback(glp_tree *T, void *info) {
  switch (glp_ios_reason(T)) {
    case GLP_ISELECT:
      // cout<<"GLP_ISELECT"<<endl;
      break;
    case GLP_IPREPRO:
      // cout<<"GLP_IPREPRO"<<endl;
      break;
    case GLP_IROWGEN:
      // cout<<"GLP_IROWGEN"<<endl;
      break;
    case GLP_IHEUR:
      // cout<<"GLP_IHEUR"<<endl;
      break;
    case GLP_ICUTGEN:
      // cout<<"GLP_ICUTGEN"<<endl;
      break;
    case GLP_IBRANCH:
      // cout<<"GLP_IBRANCH"<<endl;
      break;
    case GLP_IBINGO:
      // cout<<"GLP_IBINGO"<<endl;
      break;
    default:
      // cout<<"DEFAULT"<<endl;
      break;
  }
  return;
}

int GLPKSolution::get_status() const {
  return (is_mip ? glp_mip_status(glpk) : glp_get_status(glpk));
}

void GLPKSolution::show_error() const {
  if (!solved) {
    std::cerr << "NOT SOLVED => status: "
              << (is_mip ? glp_mip_status(glpk) : glp_get_status(glpk)) << " (";
    switch (is_mip ? glp_mip_status(glpk) : glp_get_status(glpk)) {
      case GLP_OPT:
        std::cerr << "GLP_OPT";
        break;
      case GLP_FEAS:
        std::cerr << "GLP_FEAS";
        break;
      case GLP_INFEAS:
        std::cerr << "GLP_INFEAS";
        break;
      case GLP_NOFEAS:
        std::cerr << "GLP_NOFEAS";
        break;
      case GLP_UNBND:
        std::cerr << "GLP_UNBND";
        break;
      case GLP_UNDEF:
        std::cerr << "GLP_UNDEF";
        break;
      default:
        std::cerr << "???";
    }
    if (!is_mip) {
      std::cerr << ") simplex: " << simplex_code << " (";
      switch (glp_get_status(glpk)) {
        case GLP_EBADB:
          std::cerr << "GLP_EBADB";
          break;
        case GLP_ESING:
          std::cerr << "GLP_ESING";
          break;
        case GLP_ECOND:
          std::cerr << "GLP_ECOND";
          break;
        case GLP_EBOUND:
          std::cerr << "GLP_EBOUND";
          break;
        case GLP_EFAIL:
          std::cerr << "GLP_EFAIL";
          break;
        case GLP_EOBJLL:
          std::cerr << "GLP_EOBJLL";
          break;
        case GLP_EOBJUL:
          std::cerr << "GLP_EOBJUL";
          break;
        case GLP_EITLIM:
          std::cerr << "GLP_EITLIM";
          break;
        case GLP_ENOPFS:
          std::cerr << "GLP_ENOPFS";
          break;
        case GLP_ENODFS:
          std::cerr << "GLP_ENODFS";
          break;
        default:
          std::cerr << "???";
      }
    }

    std::cerr << ")" << std::endl;
  } else {
    std::cerr << "SOLVED => status: "
              << (is_mip ? glp_mip_status(glpk) : glp_get_status(glpk)) << " (";
    switch (is_mip ? glp_mip_status(glpk) : glp_get_status(glpk)) {
      case GLP_OPT:
        std::cerr << "GLP_OPT";
        break;
      case GLP_FEAS:
        std::cerr << "GLP_FEAS";
        break;
      case GLP_INFEAS:
        std::cerr << "GLP_INFEAS";
        break;
      case GLP_NOFEAS:
        std::cerr << "GLP_NOFEAS";
        break;
      case GLP_UNBND:
        std::cerr << "GLP_UNBND";
        break;
      case GLP_UNDEF:
        std::cerr << "GLP_UNDEF";
        break;
      default:
        std::cerr << "???";
    }
    if (!is_mip) {
      std::cerr << ") simplex: " << simplex_code << " (";
      switch (glp_get_status(glpk)) {
        case GLP_EBADB:
          std::cerr << "GLP_EBADB";
          break;
        case GLP_ESING:
          std::cerr << "GLP_ESING";
          break;
        case GLP_ECOND:
          std::cerr << "GLP_ECOND";
          break;
        case GLP_EBOUND:
          std::cerr << "GLP_EBOUND";
          break;
        case GLP_EFAIL:
          std::cerr << "GLP_EFAIL";
          break;
        case GLP_EOBJLL:
          std::cerr << "GLP_EOBJLL";
          break;
        case GLP_EOBJUL:
          std::cerr << "GLP_EOBJUL";
          break;
        case GLP_EITLIM:
          std::cerr << "GLP_EITLIM";
          break;
        case GLP_ENOPFS:
          std::cerr << "GLP_ENOPFS";
          break;
        case GLP_ENODFS:
          std::cerr << "GLP_ENODFS";
          break;
        default:
          std::cerr << "???";
      }
    }

    std::cerr << ")" << std::endl;
  }
}

double GLPKSolution::get_value(unsigned int var) const {
  if (is_mip)
   return glp_mip_col_val(glpk, var + 1);
  else
    return glp_get_col_prim(glpk, var + 1);
}

double GLPKSolution::evaluate(LinearExpression exp) const {
  double sum = 0;
  foreach(exp.get_terms(), term) {
    double coeff = term->first;
    unsigned int var = term->second;
    sum += coeff * get_value(var);
  }
  return sum;
}

bool GLPKSolution::is_solved() const { return solved; }
void GLPKSolution::solve(double var_lb, double var_ub) {
#if GLPK_TERM_OUT == 0
  glp_term_out(GLP_OFF);
#else
  glp_term_out(GLP_ON);
#endif
  // cout << "DIR:" << dir << endl;
  switch (dir) {
    case 0:
      glp_set_obj_dir(glpk, GLP_MAX);
      break;
    case 1:
      glp_set_obj_dir(glpk, GLP_MIN);
      break;
    default:
      glp_set_obj_dir(glpk, GLP_MAX);
  }
        // cout << "col_num:" << col_num << endl;
  glp_add_cols(glpk, col_num);
        // cout << "row_num:" << row_num << endl;
  glp_add_rows(glpk, row_num);
  set_objective();
  set_bounds(var_lb, var_ub);
  set_coefficients();
  if (is_mip) set_column_types();
  if (is_mip) {
    glp_iocp glpk_params;
    glp_init_iocp(&glpk_params);

    // presolver is required because otherwise
    // GLPK expects glpk to hold an optimal solution
    // to the relaxed LP.

    glpk_params.presolve = GLP_ON;

    // glpk_params.br_tech = GLP_BR_FFV;//First Fractional Variable
    // glpk_params.br_tech = GLP_BR_LFV;//Last Fractional Variable
    // glpk_params.br_tech = GLP_BR_MFV;//Most Fractional Variable
    glpk_params.br_tech = GLP_BR_DTH;  // D&T Heuristic
    // glpk_params.br_tech = GLP_BR_PCH;//Pseudo-Cost Heuristic

    // glpk_params.bt_tech = GLP_BT_DFS;//Depth First Search
    glpk_params.bt_tech = GLP_BT_BFS;  // Breadth First Search
    // glpk_params.bt_tech = GLP_BT_BLB;//Best Local Branch
    // glpk_params.bt_tech = GLP_BT_BPH;//Best Projection Heuristic

    /*
    #if GLPK_TERM_OUT == 0
                    glpk_params.msg_lev = (GLP_MSG_OFF);
    #else if GLPK_TERM_OUT == 1
                    glpk_params.msg_lev = (GLP_MSG_ON);
    #endif
    */
    // glpk_params.sr_heur = GLP_OFF;

    // Feasibility pump heuristic
    // glpk_params.fp_heur = GLP_ON;

    // Proximity search heuristic
    // glpk_params.ps_heur = GLP_ON;
    // glpk_params.ps_tm_lim = 300000;

    // Gomory's mixed integer cut
    glpk_params.gmi_cuts = GLP_ON;

    glpk_params.mir_cuts = GLP_ON;

    glpk_params.cov_cuts = GLP_ON;

    glpk_params.clq_cuts = GLP_ON;

    glpk_params.mip_gap = 1;

    if (0 < t_limit) glpk_params.tm_lim = t_limit;

    // glpk_params.cb_func = callback;

    if (0 == aim) {
      solved = glp_intopt(glpk, &glpk_params) == 0 &&
               glp_mip_status(glpk) == GLP_OPT;
    } else if (1 == aim) {
      solved = glp_intopt(glpk, &glpk_params) == 0;
      int ret = glp_mip_status(glpk);
      if (ret == GLP_FEAS || ret == GLP_OPT)
        solved = true;
      else
        solved = false;
    }
  } else {
    glp_smcp glpk_params;
    glp_init_smcp(&glpk_params);

    /* Set solver options. The presolver is essential. The other two
     * options seem to make the solver slightly faster.
     *
     * Tested with GLPK 4.43 on wks-50-12.
     */
    glpk_params.presolve = GLP_ON;
    glpk_params.pricing = GLP_PT_STD;
    glpk_params.r_test = GLP_RT_STD;
    cout <<"do glp_simplex"<<endl;
    simplex_code = glp_simplex(glpk, &glpk_params);
    if (0 == aim)
      solved = simplex_code == 0 && glp_get_status(glpk) == GLP_OPT;
    else if (1 == aim)
      solved = simplex_code == 0 && (glp_get_status(glpk) == GLP_FEAS ||
                                     glp_get_status(glpk) == GLP_OPT);
  }
}

void GLPKSolution::set_objective() {
  assert(lp.get_objective()->get_terms().size() <= col_num);

  foreach(lp.get_objective()->get_terms(), term)
    glp_set_obj_coef(glpk, term->second + 1, term->first);
}

/*
        GLP_FR -8 < x < +8 Free variable
        GLP_LO lb < x < +8 Variable with lower bound
        GLP_UP -8 < x < ub Variable with upper bound
        GLP_DB lb <=x<= ub Double-bound variable
        GLP_FX lb = x = ub Fixed variable
*/
void GLPKSolution::set_bounds(double col_lb, double col_ub) {
  unsigned int r = 1;

  foreach(lp.get_equalities(), equ) {
    glp_set_row_bnds(glpk, r++, GLP_FX, equ->second, equ->second);
    coeff_num += equ->first->get_terms().size();
  }

  foreach(lp.get_inequalities(), inequ) {
    glp_set_row_bnds(glpk, r++, GLP_UP, 0, inequ->second);
    coeff_num += inequ->first->get_terms().size();
  }

  for (unsigned int c = 1; c <= col_num; c++)
    glp_set_col_bnds(glpk, c, GLP_DB, col_lb, col_ub);

  foreach(lp.get_non_default_variable_ranges(), bnds) {
    unsigned int c = bnds->var_index + 1;
    int col_type;
    col_lb = bnds->lower_bound;
    col_ub = bnds->upper_bound;

    if (bnds->has_upper && bnds->has_lower)
      col_type = GLP_DB;
    else if (!bnds->has_upper && !bnds->has_lower)
      col_type = GLP_FR;
    else if (bnds->has_upper)
      col_type = GLP_UP;
    else
      col_type = GLP_LO;
    // cout<<"var_id:"<<bnds->var_index<<" lb:"<<col_lb<<" ub:"<<col_ub<<endl;
    glp_set_col_bnds(glpk, c, col_type, col_lb, col_ub);
  }
}

/*
Construction of coefficient matrix and load it.
*/
void GLPKSolution::set_coefficients() {
  int *row_idx, *col_idx;
  double *coeff;

  row_idx = new int[1 + coeff_num];
  col_idx = new int[1 + coeff_num];
  coeff = new double[1 + coeff_num];

  unsigned int r = 1, k = 1;

  foreach(lp.get_equalities(), equ) {
    foreach(equ->first->get_terms(), term) {
      assert(k <= coeff_num);

      row_idx[k] = r;
      col_idx[k] = 1 + term->second;
      coeff[k] = term->first;

      k += 1;
    }
    r += 1;
  }

  foreach(lp.get_inequalities(), inequ) {
    foreach(inequ->first->get_terms(), term) {
      assert(k <= coeff_num);

      row_idx[k] = r;
      col_idx[k] = 1 + term->second;
      coeff[k] = term->first;

      k += 1;
    }
    r += 1;
  }
  /*
  for(uint i = 1; i <= coeff_num; i++)
  {
          cout<<"["<<row_idx[i]<<","<<col_idx[i]<<"]"<<" = "<<coeff[i]<<endl;
  }
  */
  glp_load_matrix(glpk, coeff_num, row_idx, col_idx, coeff);

  delete[] row_idx;
  delete[] col_idx;
  delete[] coeff;
}

void GLPKSolution::set_column_types() {
  unsigned int col_idx;

  foreach(lp.get_integer_variables(), var_id) {
    col_idx = 1 + *var_id;
    uint col_lb = 0;
    foreach(lp.get_non_default_variable_ranges(), bnds) {
      if (*var_id == bnds->var_index) {
        col_lb = bnds->lower_bound;
        break;
      }
    }
    // hack: for integer variables, ignore upper bound for now
    glp_set_col_bnds(glpk, col_idx, GLP_LO, col_lb, 0);
    glp_set_col_kind(glpk, col_idx, GLP_IV);
  }

  foreach(lp.get_binary_variables(), var_id) {
    col_idx = 1 + *var_id;
    glp_set_col_kind(glpk, col_idx, GLP_BV);
  }
}

void GLPKSolution::set_time_limit(int64_t time) { t_limit = time; }

int64_t GLPKSolution::get_time_limit() { return t_limit; }
