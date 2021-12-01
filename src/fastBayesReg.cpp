#include <RcppArmadillo.h>

// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::interfaces(r, cpp)]]

using namespace Rcpp;
using namespace arma;

//'@importFrom Rcpp evalCpp
//'@importFrom pgdraw pgdraw
//'@useDynLib fastBayesReg, .registration=TRUE

#define LOG2 0.693147180559945

//'@title Accurately compute log(1-exp(-x)) for x > 0
//'@param x a vector of nonnegative numbers
//'@return a vector of values of log(1-exp(-x))
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'x <- seq(0,10,length=1000)
//'y <- log1mexpm(x)
//'plot(x,y,type="l")
//'@export
// [[Rcpp::export]]
arma::vec log1mexpm(arma::vec& x){
	arma::uvec idx1 = arma::find(x<=LOG2);
	arma::uvec idx0 = arma::find(x>LOG2);
	arma::vec y;
	y.zeros(x.n_elem);
	y.elem(idx1) = arma::log(0.0 - arma::expm1(0.0-x.elem(idx1)));
	y.elem(idx0) = arma::log1p(0.0 - arma::exp(0.0-x.elem(idx0)));
	return y;
}


//'@title Accurately compute log(1+exp(x))
//'@param x a vector of real numbers
//'@return a vector of values of log(1+exp(x))
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'x <- seq(-100,100,length=1000)
//'y <- log1pexp(x)
//'plot(x,y,type="l")
//'@export
// [[Rcpp::export]]
arma::vec log1pexp(arma::vec& x){
	arma::uvec idx0 = arma::find(x<=-37);
	arma::uvec idx1 = arma::find(x>-37 && x<=18);
	arma::uvec idx2 = arma::find(x>18 && x<=33.3);
	arma::uvec idx3 = arma::find(x>33.3);
	arma::vec y;
	y.zeros(x.n_elem);
	y.elem(idx0) = arma::exp(x.elem(idx0));
	y.elem(idx1) = arma::log1p(arma::exp(x.elem(idx1)));
	y.elem(idx2) = x.elem(idx2) + arma::exp(0.0-x.elem(idx2));
	y.elem(idx3) = x.elem(idx3);
	return y;
}

//'@title Simulate data from the linear regression model
//'@param n sample size
//'@param p number of candidate predictors
//'@param q number of nonzero predictors
//'@param R2 R-squared indicating the proportion of variation explained by the predictors
//'@param beta_size effect size of beta coefficients
//'@param X_cor correlation between covariates
//'@return a list objects consisting of the following components
//'\describe{
//'\item{y}{vector of n outcome variables}
//'\item{X}{n x p matrix of candidate predictors}
//'\item{betacoef}{vector of p regression coeficients}
//'\item{R2}{R-squared indicating the proportion of variation explained by the predictors}
//'\item{sigma2}{noise variance}
//'\item{X_cor}{correlation between covariates}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'dat<-sim_linear_reg(n=25,p=2000,X_cor=0.9,q=6)
//'summary(with(dat,lm(y~X)))
//'@export
// [[Rcpp::export]]
Rcpp::List sim_linear_reg(int n = 100, int p = 20, int q = 5,
                          double R2 = 0.95, double X_cor = 0.5,
                          double beta_size = 1){
	arma::mat x = sqrt(1.0 - X_cor)*arma::randn<arma::mat>(n,p);
	arma::vec z = sqrt(X_cor)*arma::randn<arma::vec>(n);
	int qh = q/2;
	arma::vec beta_nonzero = arma::repmat(vec({beta_size,-beta_size}),qh,1);
	if(q>2*qh){
		beta_nonzero = arma::join_cols(beta_nonzero,vec({beta_size}));
	}
	for(arma::uword i=0;i<x.n_cols;i++){
		x.col(i) += z;
	}
	arma::vec y = x.cols(0,q-1L)*beta_nonzero;
	double var_y = arma::var(y);
	double sigma2 = var_y*(1.0 - R2)/R2;
	y += arma::randn(n)*sqrt(sigma2);
	return Rcpp::List::create(Named("y") = y,
                           Named("X") = x,
                           Named("betacoef") = arma::join_cols(beta_nonzero,arma::zeros<arma::vec>(p-q)),
                           Named("R2") = R2,
                           Named("sigma2") = sigma2,
                           Named("X_cor") = X_cor);
}

//'@title Simulate data from the logistic regression model
//'@param n sample size
//'@param p number of candidate predictors
//'@param q number of nonzero predictors
//'@param R2 R-squared indicating the proportion of variation explained by the predictors
//'@param beta_size effect size of beta coefficients
//'@param X_cor correlation between covariates
//'@return a list objects consisting of the following components
//'\describe{
//'\item{y}{vector of n outcome variables}
//'\item{X}{n x p matrix of candidate predictors}
//'\item{betacoef}{vector of p regression coeficients}
//'\item{R2}{R-squared indicating the proportion of variation explained by the predictors}
//'\item{sigma2}{noise variance}
//'\item{X_cor}{correlation between covariates}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'dat<-sim_logit_reg(n=1000,p=20,X_var = 1,X_cor=0.9,q=10)
//'summary(with(dat,glm(y~0+X,family=binomial())))
//'@export
// [[Rcpp::export]]
Rcpp::List sim_logit_reg(int n = 100, int p = 20, int q = 5,
                          double X_cor = 0.5, double X_var = 10,
                          double beta_size = 1){
	arma::mat x = sqrt(1.0 - X_cor)*arma::randn<arma::mat>(n,p);
	arma::vec z = sqrt(X_cor)*arma::randn<arma::vec>(n);
	int qh = q/2;
	arma::vec beta_nonzero = arma::repmat(vec({beta_size,-beta_size}),qh,1);
	if(q>2*qh){
		beta_nonzero = arma::join_cols(beta_nonzero,vec({beta_size}));
	}
	for(arma::uword i=0;i<x.n_cols;i++){
		x.col(i) += z;
	}
	x *= sqrt(X_var);
	arma::vec mu = x.cols(0,q-1L)*beta_nonzero;
	arma::vec prob = 1.0/(1+exp(-mu));
	arma::uvec y;
	y.zeros(n);
	arma::uvec idx1 = arma::find(arma::randu(n) < prob);
	y.elem(idx1).ones();
	double var_y = arma::var(arma::conv_to<arma::vec>::from(y));
	double R2 = 0.0;
	if(var_y>0)
		R2 = arma::var(prob)/var_y;

	return Rcpp::List::create(Named("y") = y,
                           Named("X") = x,
                           Named("betacoef") = arma::join_cols(beta_nonzero,arma::zeros<arma::vec>(p-q)),
                           Named("R2") = R2,
                           Named("prob") = prob,
                           Named("X_cor") = X_cor,
                           Named("X_var") = X_var);
}


void one_step_update_big_p(arma::vec& betacoef, double& sigma2_eps, double& tau2,
                     double& b_tau, arma::vec& mu, arma::vec& ys,  arma::mat& V, arma::vec& d,arma::vec& d2,
                     arma::vec& y, arma::mat& X,
                      double A2, double a_sigma, double b_sigma,
                     int p, int n){

	arma::vec alpha_1 = arma::randn<arma::vec>(p)*sqrt(sigma2_eps*tau2);
	arma::vec alpha_2 = arma::randn<arma::vec>(n)*sqrt(sigma2_eps);
	arma::vec beta_s = (ys - d%(V.t()*alpha_1) - alpha_2)%d/(1.0 + tau2*d2);
	betacoef = alpha_1 + tau2*V*beta_s;
	mu = X*betacoef;
	arma::vec eps = y - mu;
	double sum_eps2 = arma::accu(eps%eps);
	double sum_beta2 = arma::accu(betacoef%betacoef);
	double inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2/sigma2_eps)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2 + inv_tau2)));
	tau2 = 1.0/inv_tau2;
	double inv_sigma2_eps = randg<double>(distr_param(a_sigma+(n+p)/2.0, 1.0/(b_sigma+0.5*sum_beta2*inv_tau2+0.5*sum_eps2)));
	sigma2_eps = 1.0/inv_sigma2_eps;
}


void one_step_update_big_n(arma::vec& betacoef, double& sigma2_eps, double& tau2,
                         double& b_tau, arma::vec& mu, arma::vec& ys,  arma::mat& V, arma::vec& d,arma::vec& d2,
                         arma::vec& y, arma::mat& X,
                         double A2, double a_sigma, double b_sigma,
                         int p, int n){

	double inv_tau2 = 1.0/tau2;
	arma::vec alpha_1 = arma::randn<arma::vec>(p)%sqrt(sigma2_eps/(d2 + inv_tau2));
	arma::vec beta_s = d%ys/(d2 + inv_tau2) + alpha_1;
	betacoef = V*beta_s;
	mu = d%beta_s;
	arma::vec eps = ys - mu;
	double sum_eps2 = arma::accu(eps%eps);
	double sum_beta2 = arma::accu(beta_s%beta_s);
	inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2/sigma2_eps)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2 + inv_tau2)));
	tau2 = 1.0/inv_tau2;
	double inv_sigma2_eps = randg<double>(distr_param(a_sigma+p, 1.0/(b_sigma+0.5*sum_beta2*inv_tau2+0.5*sum_eps2)));
	sigma2_eps = 1.0/inv_sigma2_eps;
}

//'@title Fast Bayesian linear regression with normal priors
//'@param y vector of n outcome variables
//'@param X n x p matrix of candidate predictors
//'@param mcmc_sample number of MCMC iterations saved
//'@param burnin number of iterations before start to save
//'@param thinning number of iterations to skip between two saved iterations
//'@param a_sigma shape parameter in the inverse gamma prior of the noise variance
//'@param b_sigma rate parameter in the inverse gamma prior of the noise variance
//'@param A_tau scale parameter in the half Cauchy prior of the ratio between the coefficient variance and the noise variance
//'@return a list object consisting of two components
//'\describe{
//'\item{post_mean}{a list object of four components for posterior mean statistics}
//'\describe{
//'\item{mu}{a vector of posterior predictive mean of the n training sample}
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{sigma2_eps}{posterior mean of the noise variance}
//'\item{tau2}{posterior mean of the ratio between prior regression coefficient variances and the noise variance}
//'}
//'\item{mcmc}{a list object of three components for MCMC samples}
//'\describe{
//'\item{mu}{a vector of posterior predictive mean of the n training sample}
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{sigma2_eps}{posterior mean of the noise variance}
//'\item{tau2}{posterior mean of the ratio between prior regression coefficient variances and the noise variance}
//'}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'set.seed(2022)
//'dat1 <- sim_linear_reg(n=2000,p=200,X_cor=0.9,q=6)
//'res1 <- with(dat1,fast_normal_lm(y,X))
//'dat2 <- sim_linear_reg(n=200,p=2000,X_cor=0.9,q=6)
//'res2 <- with(dat2,fast_normal_lm(y,X))
//'tab <- data.frame(rbind(comp_sparse_SSE(dat1$betacoef,res1$post_mean$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2$post_mean$betacoef)),
//'time=c(res1$elapsed,res2$elapsed))
//'rownames(tab)<-c("n = 2000, p = 200","n = 200, p = 2000")
//'fast_normal_tab <- tab
//'print(fast_normal_tab)
//'@export
// [[Rcpp::export]]
Rcpp::List fast_normal_lm(arma::vec& y, arma::mat& X,
                         int mcmc_sample = 500,
                         int burnin = 500, int thinning = 1,
                         double a_sigma = 0.01, double b_sigma = 0.01,
                         double A_tau = 10){

	arma::wall_clock timer;
	timer.tic();
	arma::vec d;
	arma::mat U;
	arma::mat V;
	arma::svd_econ(U,d,V,X);

	//std::cout << "U (" << U.n_rows << "," << U.n_cols << ")" << std::endl;
	//std::cout << "d (" << d.n_elem  << ")" << std::endl;
	//std::cout << "V (" << V.n_rows << "," << V.n_cols << ")" << std::endl;

	int p = X.n_cols;
	int n = X.n_rows;
	double sigma2_eps = b_sigma/a_sigma;
	double A2 = A_tau*A_tau;
	double b_tau = A2;
	double tau2 = b_tau;
	arma::vec d2 = d%d;
	arma::vec ys = U.t()*y;


	arma::vec betacoef;
	arma::vec mu;

	arma::mat betacoef_list;
	arma::vec sigma2_eps_list;
	arma::vec tau2_list;

	betacoef_list.zeros(p,mcmc_sample);
	sigma2_eps_list.zeros(mcmc_sample);
	tau2_list.zeros(mcmc_sample);


	if(p<n){
		for(int iter=0;iter<burnin;iter++){
			one_step_update_big_n(betacoef, sigma2_eps, tau2,
                 b_tau, mu, ys,  V,  d, d2, y,  X,
                 A2,  a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				one_step_update_big_n(betacoef, sigma2_eps, tau2,
                          b_tau, mu, ys,  V,  d, d2, y,  X,
                          A2,  a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}
	} else{
		for(int iter=0;iter<burnin;iter++){
			one_step_update_big_p(betacoef, sigma2_eps, tau2,
                         b_tau, mu, ys,  V,  d, d2, y,  X,
                         A2,  a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				one_step_update_big_p(betacoef, sigma2_eps, tau2,
                          b_tau, mu, ys,  V,  d, d2, y,  X,
                          A2,  a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}

	}

	betacoef = arma::mean(betacoef_list,1);
	sigma2_eps = arma::mean(sigma2_eps_list);
	tau2 = arma::mean(tau2_list);

	Rcpp::List post_mean = Rcpp::List::create(Named("mu") = X*betacoef,
                                           Named("betacoef") = betacoef,
                                           Named("sigma2_eps") = sigma2_eps,
                                           Named("tau2") = tau2);
	Rcpp::List mcmc = Rcpp::List::create(Named("betacoef") = betacoef_list,
                                      Named("sigma2_eps") = sigma2_eps_list,
                                      Named("tau2") = tau2_list);

	double elapsed = timer.toc();
	return Rcpp::List::create(Named("post_mean") = post_mean,
                           Named("mcmc") = mcmc,
                           Named("elapsed") = elapsed);
}

void one_step_logit_normal_big_n(arma::vec& betacoef, double& tau2, double& b_tau,
                                 arma::vec& omega, arma::vec& mu,
                           arma::vec& y_s, arma::vec& Xty_s, arma::mat& X,
                           double A2_tau, int p, int n, Rcpp::Function& pgdraw){

	//update beta
	double inv_tau2 = 1.0/tau2;
	arma::mat OmegaX = X;
	for(int j=0;j<p;j++){
		OmegaX.col(j) %= omega;
	}
	arma::mat XtX = X.t()*OmegaX;
	XtX.diag() += inv_tau2;
	arma::mat R = arma::chol(XtX);
	arma::vec b = arma::solve(R.t(),Xty_s,solve_opts::fast);
	arma::vec alpha;
	alpha.randn(p);
	betacoef = arma::solve(R,alpha+b,solve_opts::fast);

	//update omega
	mu = X*betacoef;
	omega = Rcpp::as<arma::vec>(pgdraw(1.0,NumericVector(mu.begin(),mu.end())));

	//update tau2
	double sum_beta2 = arma::accu(betacoef%betacoef);
	inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2_tau + inv_tau2)));
	tau2 = 1.0/inv_tau2;

}

void one_step_logit_normal_big_p(arma::vec& betacoef, double& tau2, double& b_tau,
                                 arma::vec& omega, arma::vec& mu,
                                 arma::vec& y_s,  arma::mat& XXt,
                                 arma::mat& X,
                                 double A2_tau, int p, int n,
                                 Rcpp::Function& pgdraw){

	//update beta
	arma::vec inv_omega = 1.0/omega;
	arma::vec alpha1;
	alpha1.randn(p);
	alpha1 *= sqrt(tau2);
	arma::vec alpha2;
	alpha2.randn(n);
	alpha2 %= sqrt(inv_omega);
	arma::mat Omega0 = tau2*XXt;
	/*for(int i=0;i<n;i++){
		Omega0(i,i) += inv_omega(i);
	}*/
	Omega0.diag() += inv_omega;

	arma::vec beta_s = arma::solve(Omega0,y_s%inv_omega - X*alpha1 - alpha2,solve_opts::fast);
	betacoef = alpha1 + tau2*X.t()*beta_s;

	//update omega
	mu = X*betacoef;
	omega = Rcpp::as<arma::vec>(pgdraw(1.0,NumericVector(mu.begin(),mu.end())));

	//update tau2
	double sum_beta2 = arma::accu(betacoef%betacoef);
	double inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2_tau + inv_tau2)));
	tau2 = 1.0/inv_tau2;

}



//'@title Fast Bayesian logistic regression with normal priors
//'@param y vector of n binrary outcome variables taking values 0 or 1
//'@param X n x p matrix of candidate predictors
//'@param mcmc_sample number of MCMC iterations saved
//'@param burnin number of iterations before start to save
//'@param thinning number of iterations to skip between two saved iterations
//'@param A_tau scale parameter in the half Cauchy prior of the ratio between the coefficient variance and the noise variance
//'@return a list object consisting of three components
//'\describe{
//'\item{post_mean}{a list object of four components for posterior mean statistics}
//'\describe{
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{tau2}{posterior mean of the ratio between prior regression coefficient variances and the noise variance}
//'\item{mu}{a vector of posterior predictive mean for linear predictor of the n training sample}
//'\item{prob}{a vector of posterior predictive probability of the n training sample}
//'}
//'\item{mcmc}{a list object of three components for MCMC samples}
//'\describe{
//'\item{betacoef}{a matrix of MCMC samples for p regression coeficients. Each column is one MCMC sample}
//'\item{tau2}{a vector of MCMC samples of global shrinkage parameters}
//'}
//'\item{elapsed}{running time}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'set.seed(2022)
//'dat1 <- sim_logit_reg(n=2000,p=200,X_cor=0.9,X_var=10,q=10,beta_size=5)
//'res1 <- with(dat1,fast_normal_logit(y,X))
//'res1_glmnet <- with(dat1,wrap_glmnet(y,X,alpha=0.5,family=binomial()))
//'dat2 <- sim_logit_reg(n=200,p=2000,X_cor=0.9,X_var=10,q=10,beta_size=5)
//'res2 <- with(dat2,fast_normal_logit(y,X,burnin=5000))
//'res2_glmnet <- with(dat2,wrap_glmnet(y,X,alpha=0.5,family=binomial()))
//'tab <- data.frame(rbind(comp_sparse_SSE(dat1$betacoef,res1$post_mean$betacoef),
//'comp_sparse_SSE(dat1$betacoef,res1_glmnet$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2$post_mean$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2_glmnet$betacoef)),
//'time=c(res1$elapsed,res1_glmnet$elapsed,res2$elapsed,res2_glmnet$elapsed))
//'rownames(tab)<-c("n = 2000, p = 200 Bayes","n = 2000, p = 200 glmnet",
//'"n = 200, p = 2000 Bayes","n = 200, p = 2000 glmnet")
//'normal_logit_tab <- tab
//'print(normal_logit_tab)
//'@export
// [[Rcpp::export]]
Rcpp::List fast_normal_logit(arma::vec& y, arma::mat& X,
                          int mcmc_sample = 500,
                          int burnin = 500, int thinning = 1,
                          double A_tau = 1){

	arma::wall_clock timer;
	timer.tic();


	int p = X.n_cols;
	int n = X.n_rows;

	double A2_tau = A_tau*A_tau;
	double b_tau = A2_tau;
	double tau2 = b_tau;

	arma::vec y_s = y - arma::ones<arma::vec>(n)*0.5;


	Rcpp::Environment pkg = Rcpp::Environment::namespace_env("pgdraw");
	Rcpp::Function pgdraw = pkg["pgdraw"];
  Rcpp::NumericVector zeros(n,0.0);
	arma::vec betacoef;
	betacoef.zeros(p);
	arma::vec mu;
	mu.zeros(n);
	arma::vec omega = Rcpp::as<arma::vec>(pgdraw(1.0,zeros));

	arma::mat betacoef_list;
	arma::mat mu_list;
	arma::vec tau2_list;

	betacoef_list.zeros(p,mcmc_sample);
	mu_list.zeros(n,mcmc_sample);
	tau2_list.zeros(mcmc_sample);


	if(p<n){
		arma::vec Xty_s = X.t()*y_s;
		for(int iter=0;iter<burnin;iter++){
			one_step_logit_normal_big_n(betacoef, tau2, b_tau,
                               omega, mu,
                               y_s, Xty_s,  X,
                               A2_tau, p, n, pgdraw);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				one_step_logit_normal_big_n(betacoef, tau2, b_tau,
                                omega, mu,
                                y_s, Xty_s,  X,
                                A2_tau, p, n, pgdraw);
			}
			betacoef_list.col(iter) = betacoef;
			tau2_list(iter) = tau2;
		}
	} else{
		arma::mat XXt = X*X.t();
		for(int iter=0;iter<burnin;iter++){
			one_step_logit_normal_big_p(betacoef, tau2, b_tau,
                               omega, mu,
                               y_s, XXt,  X,
                               A2_tau, p, n, pgdraw);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				one_step_logit_normal_big_p(betacoef, tau2, b_tau,
                                omega, mu,
                                y_s, XXt,  X,
                                A2_tau, p, n, pgdraw);
			}
			betacoef_list.col(iter) = betacoef;
			tau2_list(iter) = tau2;
		}

	}

	betacoef = arma::mean(betacoef_list,1);
	tau2 = arma::mean(tau2_list);
	mu =  X*betacoef;

	Rcpp::List post_mean = Rcpp::List::create(Named("betacoef") = betacoef,
                                           Named("tau2") = tau2,
                                           Named("mu") = mu,
                                           Named("prob") = 1.0/(1.0+exp(-mu)));
	Rcpp::List mcmc = Rcpp::List::create(Named("betacoef") = betacoef_list,
                                      Named("tau2") = tau2_list);

	double elapsed = timer.toc();
	return Rcpp::List::create(Named("post_mean") = post_mean,
                           Named("mcmc") = mcmc,
                           Named("elapsed") = elapsed);
}

void one_step_logit_horseshoe_big_n(arma::vec& betacoef, double& tau2, double& b_tau,
                                 arma::vec& omega, arma::vec& lambda, arma::vec& b_lambda,
                                 arma::vec& mu, arma::vec& y_s, arma::vec& Xty_s, arma::mat& X,
                                 double& A2_tau, double& A2_lambda,
                                 int& p, int& n, Rcpp::Function& pgdraw){

	//update beta
	double inv_tau2 = 1.0/tau2;
	arma::vec inv_lambda2 = 1.0/(lambda%lambda);
	arma::mat OmegaX = X;
	for(int j=0;j<p;j++){
		OmegaX.col(j) %= omega;
	}
	arma::mat XtX = X.t()*OmegaX;
	XtX.diag() += inv_tau2*inv_lambda2;
	arma::mat R = arma::chol(XtX);
	arma::vec b = arma::solve(R.t(),Xty_s,solve_opts::fast);
	arma::vec alpha;
	alpha.randn(p);
	betacoef = arma::solve(R,alpha+b,solve_opts::fast);

	//update omega
	mu = X*betacoef;
	omega = Rcpp::as<arma::vec>(pgdraw(1.0,NumericVector(mu.begin(),mu.end())));

	//update tau2
	arma::vec betacoef2 = betacoef%betacoef;
	double sum_beta2_inv_lambda2 = arma::accu(betacoef2%inv_lambda2);
	inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2_inv_lambda2)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2_tau + inv_tau2)));
	tau2 = 1.0/inv_tau2;

	//update lambda
	inv_lambda2 = arma::randg<arma::vec>(p,distr_param(1.0,1.0));
	inv_lambda2 /= b_lambda + 0.5*betacoef2/tau2;
	b_lambda = randg<arma::vec>(p,distr_param(1.0, 1.0));
	b_lambda /= 1.0/A2_lambda+inv_lambda2;
	lambda = sqrt(1.0/inv_lambda2);

}

void one_step_logit_horseshoe_big_p(arma::vec& betacoef, double& tau2, double& b_tau,
                                 arma::vec& omega,arma::vec& lambda, arma::vec& b_lambda,
                                 arma::vec& mu, arma::vec& y_s, arma::mat& X,
                                 double& A2_tau, double& A2_lambda,
                                 int& p, int& n,
                                 Rcpp::Function& pgdraw){

	//update beta
	arma::vec inv_omega = 1.0/omega;
	arma::vec alpha1;
	alpha1.randn(p);
	alpha1 *= sqrt(tau2);
	alpha1 %= lambda;
	arma::vec alpha2;
	alpha2.randn(n);
	alpha2 %= sqrt(inv_omega);
	arma::mat XLambda = X;
	for(int i=0;i<n;i++){
		XLambda.row(i) %= lambda.t();
	}
	arma::mat Omega0 = XLambda*XLambda.t();

	Omega0.diag() += inv_omega/tau2;


	arma::vec beta_s = arma::solve(Omega0, y_s%inv_omega - X*alpha1 - alpha2);
	betacoef = alpha1 + lambda%(XLambda.t()*beta_s);

	//update omega
	mu = X*betacoef;
	omega = Rcpp::as<arma::vec>(pgdraw(1.0,NumericVector(mu.begin(),mu.end())));

	//update lambda
	//arma::vec betacoef2 = betacoef%betacoef;

	arma::vec inv_lambda2 = 1.0/(lambda%lambda);
	// arma::vec B = 0.5*betacoef2/tau2;
	// b_lambda = arma::randu<arma::vec>(p)%(A2_lambda/(1.0+A2_lambda*inv_lambda2));
	// arma::vec upsilon = arma::randu<arma::vec>(p);
	// arma::vec C = 1.0/b_lambda - 1.0/A2_lambda;
	// inv_lambda2 = -arma::log1p(-upsilon%(1.0 - exp(-B%C)))/B;

	// arma::vec inv_lambda2 = arma::randg<arma::vec>(p,distr_param(1.0,1.0));
	// inv_lambda2 /= b_lambda + 0.5*betacoef2/tau2;
	// b_lambda = randg<arma::vec>(p,distr_param(1.0, 1.0));
	// b_lambda /= 1.0/A2_lambda+inv_lambda2;

	//lambda = sqrt(1.0/inv_lambda2);


	//update tau2
	arma::vec betacoef2 = betacoef%betacoef;
	double sum_beta2_inv_lambda2 = arma::accu(betacoef2%inv_lambda2);
	double inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2_inv_lambda2)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2_tau + inv_tau2)));
	tau2 = 1.0/inv_tau2;

	//update lambda
	inv_lambda2 = arma::randg<arma::vec>(p,distr_param(1.0,1.0));
	inv_lambda2 /= b_lambda + 0.5*betacoef2/tau2;
	b_lambda = randg<arma::vec>(p,distr_param(1.0, 1.0));
	b_lambda /= 1.0/A2_lambda+inv_lambda2;
	lambda = sqrt(1.0/inv_lambda2);

}



//'@title Fast Bayesian logistic regression with horseshoe priors
//'@param y vector of n binary outcome variables taking values 0 or 1
//'@param X n x p matrix of candidate predictors
//'@param mcmc_sample number of MCMC iterations saved
//'@param burnin number of iterations before start to save
//'@param thinning number of iterations to skip between two saved iterations
//'@param A_tau scale parameter in the half Cauchy prior of the ratio between the coefficient variance and the noise variance
//'@return a list object consisting of three components
//'\describe{
//'\item{post_mean}{a list object of five components for posterior mean statistics}
//'\describe{
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{tau2}{posterior mean of the ratio between prior regression coefficient variances and the noise variance}
//'\item{lambda}{a vector of posterior mean of p local shrinkage parameters}
//'\item{mu}{a vector of posterior predictive mean for linear predictor of the n training sample}
//'\item{prob}{a vector of posterior predictive probability of the n training sample}
//'}
//'\item{mcmc}{a list object of three components for MCMC samples}
//'\describe{
//'\item{betacoef}{a matrix of MCMC samples for p regression coeficients. Each column is one MCMC sample}
//'\item{tau2}{a vector of MCMC samples of global shrinkage parameters}
//'\item{lambda}{a matrix of MCMC samples of p local shrinkage parameters. Each column is one MCMC sample}
//'}
//'\item{elapsed}{running time}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'set.seed(2022)
//'dat1 <- sim_logit_reg(n=2000,p=200,X_cor=0.9,X_var=10,q=10,beta_size=5)
//'res1 <- with(dat1,fast_horseshoe_logit(y,X,A_tau=0.001,A_lambda=0.001))
//'res1_glmnet <- with(dat1,wrap_glmnet(y,X,alpha=0.5,family=binomial()))
//'dat2 <- sim_logit_reg(n=200,p=2000,X_cor=0.9,X_var=10,q=10,beta_size=5)
//'res2 <- with(dat2,fast_horseshoe_logit(y,X,burnin=5000,mcmc_sample=500,A_tau=0.001,A_lambda=0.001))
//'res2_glmnet <- with(dat2,wrap_glmnet(y,X,alpha=0.5,family=binomial()))
//'tab <- data.frame(rbind(comp_sparse_SSE(dat1$betacoef,res1$post_mean$betacoef),
//'comp_sparse_SSE(dat1$betacoef,res1_glmnet$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2$post_mean$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2_glmnet$betacoef)),
//'time=c(res1$elapsed,res1_glmnet$elapsed,res2$elapsed,res2_glmnet$elapsed))
//'rownames(tab)<-c("n = 2000, p = 200 Bayes","n = 2000, p = 200 glmnet",
//'"n = 200, p = 2000 Bayes","n = 200, p = 2000 glmnet")
//'normal_logit_tab <- tab
//'print(normal_logit_tab)
//'@export
// [[Rcpp::export]]
Rcpp::List fast_horseshoe_logit(arma::vec& y, arma::mat& X,
                             int mcmc_sample = 500,
                             int burnin = 500, int thinning = 1,
                             double A_tau = 1, double A_lambda = 1){

	arma::wall_clock timer;
	timer.tic();


	int p = X.n_cols;
	int n = X.n_rows;

	double A2_tau = A_tau*A_tau;
	double b_tau = A2_tau;
	double tau2 = b_tau;
	double A2_lambda = A_lambda*A_lambda;

	arma::vec y_s = y - arma::ones<arma::vec>(n)*0.5;


	Rcpp::Environment pkg = Rcpp::Environment::namespace_env("pgdraw");
	Rcpp::Function pgdraw = pkg["pgdraw"];
	Rcpp::NumericVector zeros(n,0.0);
	arma::vec betacoef;
	arma::vec lambda;
	arma::vec b_lambda;
	arma::vec mu;

	lambda.ones(p);
	b_lambda.ones(p);
	betacoef.zeros(p);
	mu.zeros(n);
	arma::vec omega = Rcpp::as<arma::vec>(pgdraw(1.0,zeros));

	arma::mat betacoef_list;
	arma::mat lambda_list;
	arma::vec tau2_list;

	betacoef_list.zeros(p,mcmc_sample);
	lambda_list.zeros(p,mcmc_sample);
	tau2_list.zeros(mcmc_sample);


	if(p<n){
		arma::vec Xty_s = X.t()*y_s;
		for(int iter=0;iter<burnin;iter++){
			one_step_logit_horseshoe_big_n(betacoef, tau2, b_tau,
                               omega, lambda, b_lambda,mu,
                               y_s, Xty_s,  X,
                               A2_tau, A2_lambda, p, n, pgdraw);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				one_step_logit_horseshoe_big_n(betacoef, tau2, b_tau,
                                   omega, lambda, b_lambda,mu,
                                   y_s, Xty_s,  X,
                                   A2_tau, A2_lambda, p, n, pgdraw);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			tau2_list(iter) = tau2;
		}
	} else{
		//arma::mat XXt = X*X.t();
		tau2 = 1.0/p;
		for(int iter=0;iter<burnin;iter++){
			one_step_logit_horseshoe_big_p(betacoef, tau2, b_tau,
                               omega, lambda, b_lambda, mu,
                               y_s,  X,
                               A2_tau, A2_lambda, p, n, pgdraw);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				one_step_logit_horseshoe_big_p(betacoef, tau2, b_tau,
                                   omega,lambda, b_lambda, mu,
                                   y_s,  X,
                                   A2_tau, A2_lambda, p, n, pgdraw);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			tau2_list(iter) = tau2;
		}

	}

	betacoef = arma::mean(betacoef_list,1);
	tau2 = arma::mean(tau2_list);
	mu =  X*betacoef;

	Rcpp::List post_mean = Rcpp::List::create(Named("betacoef") = betacoef,
                                           Named("tau2") = tau2,
                                           Named("lambda") = lambda,
                                           Named("mu") = mu,
                                           Named("prob") = 1.0/(1.0+exp(-mu)));
	Rcpp::List mcmc = Rcpp::List::create(Named("betacoef") = betacoef_list,
                                      Named("tau2") = tau2_list,
                                      Named("lambda") = lambda_list);

	double elapsed = timer.toc();
	return Rcpp::List::create(Named("post_mean") = post_mean,
                           Named("mcmc") = mcmc,
                           Named("elapsed") = elapsed);
}


//'@title Simulate left standard truncated normal distribution
//'@param n sample size
//'@param lower the lower bound
//'@return a vector of random numbers from a left standard truncated
//'normal distribution
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'r0 <- rand_left_trucnorm0(1000, lower=2)
//'hist(r0)
//'@export
// [[Rcpp::export]]
arma::vec rand_left_trucnorm0(int n, double lower, double ratio=1.0){
	arma::vec y = arma::zeros<arma::vec>(n);
	int m = 0;
	if(lower<=0){
		arma::vec z = arma::randn<arma::vec>(ceil(ratio*(n - m)));
		arma::uvec idx = arma::find(z>lower);
		m += idx.n_elem;
		if(idx.n_elem>0){
		if(m<=n)
			y.subvec(0,m-1) = z.elem(idx);
		else
			y = z.elem(idx.subvec(0,n-1));
		}
		while(m < n){
			z = arma::randn<arma::vec>(ceil(ratio*(n - m)));
			idx = arma::find(z>lower);
			if(idx.n_elem>0){
				if(m + idx.n_elem <= n)
					y.subvec(m, m + idx.n_elem - 1) = z.elem(idx);
				else
					y.subvec(m,n-1) = z.elem(idx.subvec(0,n-m-1));
				m += idx.n_elem;
			}
		}
	} else{
		double alpha_star = 0.5*(lower+sqrt(lower*lower+4.0));
		arma::vec z = lower - log(arma::randu<arma::vec>(ceil(ratio*n)))/alpha_star;
    arma::vec log_rho_z = (z - alpha_star);
    log_rho_z %= -0.5*log_rho_z;
    arma::vec u = arma::randu<arma::vec>(ceil(ratio*n));
    arma::uvec idx = arma::find(log(u) < log_rho_z);
    m += idx.n_elem;
    if(idx.n_elem>0){
    if(m<=n)
    	y.subvec(0,m-1) = z.elem(idx);
    else
    	y = z.elem(idx.subvec(0,n-1));
    }
    while(m < n){
    	z = lower - log(arma::randu<arma::vec>(ceil(ratio*(n - m))))/alpha_star;
    	log_rho_z = (z - alpha_star);
    	log_rho_z %= -0.5*log_rho_z;
    	u = arma::randu<arma::vec>(ceil(ratio*(n - m)));
    	idx = arma::find(log(u) < log_rho_z);
    	if(idx.n_elem>0){
    		if(m + idx.n_elem <= n)
    			y.subvec(m, m + idx.n_elem - 1) = z.elem(idx);
    		else
    			y.subvec(m,n-1) = z.elem(idx.subvec(0,n-m-1));
    		m += idx.n_elem;
    	}
    }
	}
	return y;
}


//'@title Simulate left truncated normal distribution
//'@param n sample size
//'@param mu the location parameter
//'@param sigma the scale parameter
//'@param lower the lower bound
//'@return a vector of random numbers from a left truncated
//'normal distribution
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'r <- rand_left_trucnorm(1000, mu=100, sigma=0.1, lower=200)
//'hist(r)
//'@export
// [[Rcpp::export]]
arma::vec rand_left_trucnorm(int n, double mu, double sigma,
                             double lower, double ratio=1.0){
	double lower0 = (lower - mu)/sigma;
	arma::vec y = rand_left_trucnorm0(n,lower0,ratio);
	y *= sigma;
	y += mu;
	return y;
}

//'@title Simulate right truncated normal distribution
//'@param n sample size
//'@param mu the location parameter
//'@param sigma the scale parameter
//'@param upper the upper bound
//'@return a vector of random numbers from a right truncated
//'normal distribution
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'r <- rand_right_trucnorm(1000, mu=100, sigma=0.1, upper=90)
//'hist(r)
//'@export
// [[Rcpp::export]]
arma::vec rand_right_trucnorm(int n, double mu, double sigma,
                             double upper, double ratio=1.0){
	arma::vec y = rand_left_trucnorm(n,-mu,sigma,-upper,ratio);
	return -y;
}


void hs_one_step_update_big_p(arma::vec& betacoef, arma::vec& lambda,
                              double& sigma2_eps, double& tau2,
                           double& b_tau, arma::vec& b_lambda, arma::vec& mu, arma::vec& ys,  arma::mat& V, arma::vec& d,arma::vec& d2,
                           arma::vec& y, arma::mat& X, arma::mat& VD,
                           double& A2_tau, double& A2_lambda,
                           double a_sigma, double b_sigma,
                           int p, int n){

	arma::vec lambda2 = lambda%lambda;
	double sigma_eps = sqrt(sigma2_eps);
	double tau = sqrt(tau2);
	double inv_tau2 = 1.0/tau2;
	arma::vec alpha_1 = arma::randn<arma::vec>(p)%lambda*sigma_eps*tau;
	arma::vec alpha_2 = arma::randn<arma::vec>(n)*sigma_eps;
	arma::mat LambdaVD = VD;
	for(int i=0;i<n;i++){
		LambdaVD.col(i) %= lambda;
	}
	arma::mat Z = LambdaVD.t()*LambdaVD;
	Z.diag() += inv_tau2;
	arma::vec beta_s = arma::solve(Z,ys - VD.t()*alpha_1 - alpha_2);
	//std::cout << beta_s.subvec(0,1) << std::endl;
	betacoef = alpha_1 + lambda2%(VD*beta_s);

	//update lambda
	arma::vec betacoef2 = betacoef%betacoef;
	arma::vec inv_lambda2 = arma::randg<arma::vec>(p,distr_param(1.0,1.0));
	inv_lambda2 /= b_lambda + 0.5*betacoef2/tau2/sigma2_eps;
	b_lambda = randg<arma::vec>(p,distr_param(1.0, 1.0));
	b_lambda /= 1.0/A2_lambda+inv_lambda2;
	lambda = sqrt(1.0/inv_lambda2);

	//update lambda
	// arma::vec betacoef2 = betacoef%betacoef;
	// arma::vec B = 0.5*betacoef2/tau2/sigma2_eps;
	// arma::vec inv_lambda2 = 1.0/(lambda%lambda);
	// b_lambda = arma::randu<arma::vec>(p)%(A2_lambda/(1.0+A2_lambda*inv_lambda2));
	// arma::vec upsilon = arma::randu<arma::vec>(p);
	// arma::vec C = 1.0/b_lambda - 1.0/A2_lambda;
	// inv_lambda2 = -arma::log1p(-upsilon%(1.0 - exp(-B%C)))/B;
	// lambda = sqrt(1.0/inv_lambda2);


	//update tau2, sigma2_eps, b_tau and b_lambda

	double sum_beta2_inv_lambda2 = arma::accu(betacoef2%inv_lambda2);

	inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2_inv_lambda2/sigma2_eps)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2_tau + inv_tau2)));
  //inv_tau2 = 1e16;
	/*double B_tau = 0.5*sum_beta2_inv_lambda2/sigma2_eps;
	double u_tau = arma::randu()*(A2_tau/(1.0+A2_tau*inv_tau2));
	double C_tau = 1.0/u_tau - 1.0/A2_tau;
	double v_tau = arma::randu()*exp(-B_tau*inv_tau2);
	double D_tau = -log(v_tau)/B_tau;
	if(D_tau > C_tau)
		D_tau = C_tau;
	double s_tau = arma::randu();
	inv_tau2 = D_tau*pow(s_tau,2.0/(p+1.0));*/
	tau2 = 1.0/inv_tau2;

	mu = X*betacoef;
	arma::vec eps = y - mu;
	double sum_eps2 = arma::accu(eps%eps);
	double inv_sigma2_eps = arma::randg<double>(distr_param(a_sigma+(p+n)/2, 1.0/(b_sigma+0.5*sum_beta2_inv_lambda2*inv_tau2+0.5*sum_eps2)));
	sigma2_eps = 1.0/inv_sigma2_eps;
}

//hs_one_step_update_big_n(betacoef,lambda, sigma2_eps, tau2,b_lambda,
//b_tau, mu, dys,  V,  d, d2, y, X,
//A2, A2_lambda, a_sigma,  b_sigma, p,  n);

void hs_one_step_update_big_n(arma::vec& betacoef,
                              arma::vec& lambda,
                              double& sigma2_eps, double& tau2, arma::vec& b_lambda,
                           double& b_tau, arma::vec& mu, arma::vec& dys,
                           arma::mat& V, arma::vec& d2,
                           arma::vec& y, arma::mat& X,
                           double A2, double A2_lambda,
                           double a_sigma, double b_sigma,
                           int p, int n){

	double inv_tau2 = 1.0/tau2;
	//double tau = sqrt(tau2);
	double sigma_eps = sqrt(sigma2_eps);
	arma::vec lambda_tau = lambda*sqrt(tau2);
	arma::mat V_d_lambda = V;
	V_d_lambda.each_col() /= lambda_tau;

	arma::mat VtV = V_d_lambda.t()*V_d_lambda;
	VtV.diag() += d2;
	//VtV /= sigma2_eps;

	arma::mat R = arma::chol(VtV);
	arma::vec b = arma::solve(R.t(),dys/sigma_eps,solve_opts::fast);
	arma::vec alpha;
	alpha.randn(p);
	betacoef = sigma_eps*V*arma::solve(R,alpha+b,solve_opts::fast);



	/*arma::vec taulambda= tau*lambda;
	arma::vec tau2lambda2 = taulambda%taulambda;
	arma::vec alpha_1 = arma::randn<arma::vec>(p)/d*sigma_eps;
	arma::vec alpha_2 = arma::randn<arma::vec>(p)%taulambda*sigma_eps;
	arma::vec ts = tau2lambda2%Xty;
	arma::vec Valpha_1 = V*alpha_1;
	arma::mat Z = XtX_inv;
	Z.diag() += tau2lambda2;
	arma::vec alpha = arma::solve(Z,ts - Valpha_1 - alpha_2)/sigma2_eps;
	betacoef = Valpha_1 + sigma2_eps*XtX_inv*alpha;
	 */
	arma::vec betacoef2 = betacoef%betacoef;
	arma::vec inv_lambda2 = arma::randg<arma::vec>(p,distr_param(1.0,1.0));
  inv_lambda2 /= b_lambda + 0.5*betacoef2/tau2/sigma2_eps;
  lambda = sqrt(1.0/inv_lambda2);
  b_lambda = randg<arma::vec>(p,distr_param(1.0, 1.0));
  b_lambda /= 1.0/A2_lambda+inv_lambda2;
  mu = X*betacoef;
	arma::vec eps = y - mu;
	double sum_eps2 = arma::accu(eps%eps);
	double sum_beta2_inv_lambda2 = arma::accu(betacoef%betacoef%inv_lambda2);
	//double sum_inv_lambda2 = arma::accu(inv_lambda2);
	inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2_inv_lambda2/sigma2_eps)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2 + inv_tau2)));
	tau2 = 1.0/inv_tau2;
	double inv_sigma2_eps = arma::randg<double>(distr_param(a_sigma+(p+n)/2, 1.0/(b_sigma+0.5*sum_beta2_inv_lambda2*inv_tau2+0.5*sum_eps2)));
	sigma2_eps = 1.0/inv_sigma2_eps;

	//std::cout << tau2 << std::endl;
	//std::cout << arma::accu(lambda) << std::endl;
}

//'@title Fast Bayesian linear regression with horseshoe priors
//'@param y vector of n outcome variables
//'@param X n x p matrix of candidate predictors
//'@param mcmc_sample number of MCMC iterations saved
//'@param burnin number of iterations before start to save
//'@param thinning number of iterations to skip between two saved iterations
//'@param a_sigma shape parameter in the inverse gamma prior of the noise variance
//'@param b_sigma rate parameter in the inverse gamma prior of the noise variance
//'@param A_tau scale parameter in the half Cauchy prior of the global shrinkage parameter
//'@param A_lambda scale parameter in the half Cauchy prior of the local shrinkage parameter
//'@return a list object consisting of two components
//'\describe{
//'\item{post_mean}{a list object of four components for posterior mean statistics}
//'\describe{
//'\item{mu}{a vector of posterior predictive mean of the n training sample}
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{lambda}{a vector of posterior mean of p local shrinkage parameters}
//'\item{sigma2_eps}{posterior mean of the noise variance}
//'\item{b_lambda}{posterior mean of the rate parameter in the prior for local shrinkage parameters}
//'\item{tau2}{posterior mean of the global parameter}
//'}
//'\item{mcmc}{a list object of three components for MCMC samples}
//'\describe{
//'\item{betacoef}{a matrix of MCMC samples of p regression coeficients}
//'\item{lambda}{a matrix of MCMC samples of p local shrinkage parameters}
//'\item{sigma2_eps}{a vector of MCMC samples of the noise variance}
//'\item{b_lambda}{a vector of MCMC samples of the rate parameter in the prior for local shrinkage parameters}
//'\item{tau2}{a vector of MCMC samples of the global shrinkage parameter}
//'}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'set.seed(2022)
//'dat1 <- sim_linear_reg(n=2000,p=200,X_cor=0.9,q=6)
//'res1 <- with(dat1,fast_horseshoe_lm(y,X))
//'dat2 <- sim_linear_reg(n=200,p=2000,X_cor=0.9,q=6)
//'res2 <- with(dat2,fast_horseshoe_lm(y,X))
//'tab <- data.frame(rbind(comp_sparse_SSE(dat1$betacoef,res1$post_mean$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2$post_mean$betacoef)),
//'time=c(res1$elapsed,res2$elapsed))
//'rownames(tab)<-c("n = 2000, p = 200","n = 200, p = 2000")
//'fast_horseshoe_tab <- tab
//'print(fast_horseshoe_tab)
//'@export
// [[Rcpp::export]]
Rcpp::List fast_horseshoe_lm(arma::vec& y, arma::mat& X,
                         int mcmc_sample = 500,
                         int burnin = 500, int thinning = 1,
                         double a_sigma = 0.0, double b_sigma = 0.0,
                         double A_tau = 1, double A_lambda = 1){

	arma::wall_clock timer;
	timer.tic();
	arma::vec d;
	arma::mat U;
	arma::mat V;
	arma::svd_econ(U,d,V,X);

	int p = X.n_cols;
	int n = X.n_rows;
	double sigma2_eps = 1;
	if(a_sigma!=0.0){
		sigma2_eps = b_sigma/a_sigma;
	}
	double A2 = A_tau*A_tau;
	double A2_lambda = A_lambda*A_lambda;
	double b_tau = 1;

	double tau2 = 1.0/p;
	arma::vec d2 = d%d;
	arma::vec ys = U.t()*y;
	arma::vec dys = d%ys;



	arma::vec betacoef;
	arma::vec lambda;
	arma::vec b_lambda;
	lambda.ones(p);
	b_lambda.ones(p);
	arma::vec mu;

	arma::mat betacoef_list;
	arma::mat lambda_list;
	arma::vec sigma2_eps_list;
	arma::vec tau2_list;

	betacoef_list.zeros(p,mcmc_sample);
	lambda_list.zeros(p,mcmc_sample);
	sigma2_eps_list.zeros(mcmc_sample);
	tau2_list.zeros(mcmc_sample);


	if(p<n){


		for(int iter=0;iter<burnin;iter++){
			hs_one_step_update_big_n(betacoef,lambda, sigma2_eps, tau2,b_lambda,
                         b_tau, mu, dys,  V,   d2, y, X,
                         A2, A2_lambda, a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				hs_one_step_update_big_n(betacoef,lambda, sigma2_eps, tau2,b_lambda,
                             b_tau, mu, dys,  V,  d2, y, X, A2,
                             A2_lambda, a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}
	} else{
		arma::mat VD = V;
		for(int j=0;j<p;j++){
			VD.row(j) %= d.t();
		}
		for(int iter=0;iter<burnin;iter++){
			hs_one_step_update_big_p(betacoef, lambda, sigma2_eps, tau2,
                         b_tau, b_lambda, mu, ys,  V,  d, d2, y,  X, VD,
                         A2, A2_lambda, a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				hs_one_step_update_big_p(betacoef, lambda, sigma2_eps, tau2,
                          b_tau,b_lambda, mu, ys,  V,  d, d2, y,  X, VD,
                          A2, A2_lambda, a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}

	}

	betacoef = arma::mean(betacoef_list,1);
	lambda = arma::mean(lambda_list,1);
	sigma2_eps = arma::mean(sigma2_eps_list);
	tau2 = arma::mean(tau2_list);

	Rcpp::List post_mean = Rcpp::List::create(Named("mu") = X*betacoef,
                                           Named("betacoef") = betacoef,
                                           Named("lambda") = lambda,
                                           Named("sigma2_eps") = sigma2_eps,
                                           Named("tau2") = tau2);
	Rcpp::List mcmc = Rcpp::List::create(Named("betacoef") = betacoef_list,
                                      Named("lambda") = lambda_list,
                                      Named("sigma2_eps") = sigma2_eps_list,
                                      Named("tau2") = tau2_list);

	double elapsed = timer.toc();
	return Rcpp::List::create(Named("post_mean") = post_mean,
                           Named("mcmc") = mcmc,
                           Named("elapsed") = elapsed);
}


void hs_one_step_update(arma::vec& betacoef, arma::vec& lambda,
                              double& sigma2_eps, double& tau2,
                              double& b_tau, arma::vec& b_lambda, arma::vec& mu,
                              arma::vec& y, arma::mat& X,
                              double& A2, double& A2_lambda,
                              double a_sigma, double b_sigma,
                              int p, int n){

	double sigma_eps = sqrt(sigma2_eps);
	double tau = sqrt(tau2);
	double inv_tau2 = 1.0/tau2;
	arma::vec alpha_1 = arma::randn<arma::vec>(p)%lambda*sigma_eps*tau;
	arma::vec alpha_2 = arma::randn<arma::vec>(n)*sigma_eps;
	arma::mat XLambda = X;
	for(int i=0;i<n;i++){
		XLambda.row(i) %= lambda.t();
	}
	arma::mat Z = XLambda*XLambda.t();
	Z.diag() += inv_tau2;
	arma::vec beta_s = arma::solve(Z,y - X*alpha_1 - alpha_2,arma::solve_opts::fast);
	//std::cout << beta_s.subvec(0,1) << std::endl;
	betacoef = alpha_1 + lambda%(XLambda.t()*beta_s);
	//update lambda
	arma::vec betacoef2 = betacoef%betacoef;
	arma::vec inv_lambda2 = arma::randg<arma::vec>(p,distr_param(1.0,1.0));
	inv_lambda2 /= b_lambda + 0.5*betacoef2/tau2/sigma2_eps;
	b_lambda = randg<arma::vec>(p,distr_param(1.0, 1.0));
	b_lambda /= 1.0/A2_lambda+inv_lambda2;
	lambda = sqrt(1.0/inv_lambda2);
	//update tau2, sigma2_eps, b_tau and b_lambda
	mu = X*betacoef;
	arma::vec eps = y - mu;
	double sum_eps2 = arma::accu(eps%eps);
	double sum_beta2_inv_lambda2 = arma::accu(betacoef2%inv_lambda2);
	//double sum_inv_lambda2 = arma::accu(inv_lambda2);
	double inv_sigma2_eps = arma::randg<double>(distr_param(a_sigma+(p+n)/2, 1.0/(b_sigma+0.5*sum_beta2_inv_lambda2*inv_tau2+0.5*sum_eps2)));
	sigma2_eps = 1.0/inv_sigma2_eps;
	inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(b_tau+0.5*sum_beta2_inv_lambda2*inv_sigma2_eps)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2 + inv_tau2)));
	tau2 = 1.0/inv_tau2;

}

void hs_one_step_update_slice_sampler(arma::vec& betacoef, arma::vec& lambda,
                        double& sigma2_eps, double& tau2,
                        double& u_tau, arma::vec& u_lambda, arma::vec& mu,
                        arma::vec& y, arma::mat& X,
                        double& A2_tau, double& A2_lambda,
                        double a_sigma, double b_sigma,
                        int p, int n){

	double sigma_eps = sqrt(sigma2_eps);
	double tau = sqrt(tau2);
	double inv_tau2 = 1.0/tau2;
	arma::vec alpha_1 = arma::randn<arma::vec>(p)%lambda*sigma_eps*tau;
	arma::vec alpha_2 = arma::randn<arma::vec>(n)*sigma_eps;
	arma::mat XLambda = X;
	for(int i=0;i<n;i++){
		XLambda.row(i) %= lambda.t();
	}
	arma::mat Z = XLambda*XLambda.t();
	Z.diag() += inv_tau2;
	arma::vec beta_s = arma::solve(Z,y - X*alpha_1 - alpha_2,arma::solve_opts::fast);
	//std::cout << beta_s.subvec(0,1) << std::endl;
	betacoef = alpha_1 + lambda%(XLambda.t()*beta_s);

	//update lambda
	arma::vec betacoef2 = betacoef%betacoef;
	arma::vec B = 0.5*betacoef2/tau2/sigma2_eps;
	arma::vec inv_lambda2 = 1.0/(lambda%lambda);
	u_lambda = arma::randu<arma::vec>(p)%(A2_lambda/(1.0+A2_lambda*inv_lambda2));
  arma::vec upsilon = arma::randu<arma::vec>(p);
  arma::vec C = 1.0/u_lambda - 1.0/A2_lambda;
  inv_lambda2 = -arma::log1p(-upsilon%(1.0 - exp(-B%C)))/B;
  //std::cout << inv_lambda2 << std::endl;
	lambda = sqrt(1.0/inv_lambda2);

	//update tau2,  b_tau

	double sum_beta2_inv_lambda2 = arma::accu(betacoef2%inv_lambda2);
	double B_tau = 0.5*sum_beta2_inv_lambda2/sigma2_eps;
	u_tau = arma::randu()*(A2_tau/(1.0+A2_tau*inv_tau2));
	double C_tau = 1.0/u_tau - 1.0/A2_tau;

	Rcpp::Environment pkg = Rcpp::Environment::namespace_env("stats");
	Rcpp::Function pgamma = pkg["pgamma"];
	Rcpp::Function qgamma = pkg["qgamma"];
	Rcpp::NumericVector F_C_tau = pgamma(C_tau,(p+1.0)/2.0,B_tau);
	Rcpp::NumericVector inv_F_tau = qgamma(arma::randu()*F_C_tau,(p+1.0)/2.0,B_tau);
	inv_tau2 = inv_F_tau(0);
	//double m_log_v_tau_B_tau = -log(arma::randu())/B_tau + inv_tau2;
	//double D_tau = m_log_v_tau_B_tau;
	//if(D_tau > C_tau)
	//	D_tau = C_tau;
	//double s_tau = arma::randu();
	//inv_tau2 = D_tau*pow(s_tau,2.0/(p+1.0));


	//std::cout << inv_tau2 << std::endl;

	//inv_tau2 = randg<double>(distr_param((1.0+p)/2.0,1.0/(u_tau+0.5*sum_beta2_inv_lambda2/sigma2_eps)));
	//u_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2_tau + inv_tau2)));

	tau2 = 1.0/inv_tau2;


	//update sigma2_eps,
	mu = X*betacoef;
	arma::vec eps = y - mu;
	double sum_eps2 = arma::accu(eps%eps);
	double inv_sigma2_eps = arma::randg<double>(distr_param(a_sigma+(p+n)/2, 1.0/(b_sigma+0.5*sum_beta2_inv_lambda2*inv_tau2+0.5*sum_eps2)));
	sigma2_eps = 1.0/inv_sigma2_eps;

}

//'@title Fast Bayesian high-dimensional linear regression with horseshoe priors using slice sampler
//'@param y vector of n outcome variables
//'@param X n x p matrix of candidate predictors
//'@param mcmc_sample number of MCMC iterations saved
//'@param burnin number of iterations before start to save
//'@param thinning number of iterations to skip between two saved iterations
//'@param a_sigma shape parameter in the inverse gamma prior of the noise variance
//'@param b_sigma rate parameter in the inverse gamma prior of the noise variance
//'@param A_tau scale parameter in the half Cauchy prior of the global shrinkage parameter
//'@param A_lambda scale parameter in the half Cauchy prior of the local shrinkage parameter
//'@return a list object consisting of two components
//'\describe{
//'\item{post_mean}{a list object of four components for posterior mean statistics}
//'\describe{
//'\item{mu}{a vector of posterior predictive mean of the n training sample}
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{lambda}{a vector of posterior mean of p local shrinkage parameters}
//'\item{sigma2_eps}{posterior mean of the noise variance}
//'\item{tau2}{posterior mean of the global parameter}
//'}
//'\item{mcmc}{a list object of three components for MCMC samples}
//'\describe{
//'\item{betacoef}{a matrix of MCMC samples of p regression coeficients}
//'\item{lambda}{a matrix of MCMC samples of p local shrinkage parameters}
//'\item{sigma2_eps}{a vector of MCMC samples of the noise variance}
//'\item{tau2}{a vector of MCMC samples of the global shrinkage parameter}
//'}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'set.seed(2022)
//'dat1 <- sim_linear_reg(n=2000,p=200,X_cor=0.9,q=6)
//'res1 <- with(dat1,fast_horseshoe_ss_lm(y,X))
//'dat2 <- sim_linear_reg(n=200,p=2000,X_cor=0.9,q=6)
//'res2 <- with(dat2,fast_horseshoe_ss_lm(y,X))
//'tab <- data.frame(rbind(comp_sparse_SSE(dat1$betacoef,res1$post_mean$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2$post_mean$betacoef)),
//'time=c(res1$elapsed,res2$elapsed))
//'rownames(tab)<-c("n = 2000, p = 200","n = 200, p = 2000")
//'fast_horseshoe_tab <- tab
//'print(fast_horseshoe_tab)
//'@export
// [[Rcpp::export]]
Rcpp::List fast_horseshoe_ss_lm(arma::vec& y, arma::mat& X,
                                int mcmc_sample = 500,
                                int burnin = 500, int thinning = 1,
                                double a_sigma = 0.0, double b_sigma = 0.0,
                                double A_tau = 1, double A_lambda = 1){

	arma::wall_clock timer;
	timer.tic();

	int p = X.n_cols;
	int n = X.n_rows;
	double sigma2_eps = 1;
	if(a_sigma!=0.0){
		sigma2_eps = b_sigma/a_sigma;
	}
	double A2 = A_tau*A_tau;
	double A2_lambda = A_lambda*A_lambda;
	double b_tau = 1;

	double tau2 = 1.0/p;




	arma::vec betacoef;
	arma::vec lambda;
	arma::vec b_lambda;
	lambda.ones(p);
	b_lambda.ones(p);
	arma::vec mu;

	arma::mat betacoef_list;
	arma::mat lambda_list;
	arma::vec sigma2_eps_list;
	arma::vec tau2_list;

	betacoef_list.zeros(p,mcmc_sample);
	lambda_list.zeros(p,mcmc_sample);
	sigma2_eps_list.zeros(mcmc_sample);
	tau2_list.zeros(mcmc_sample);


	if(p<n){
		arma::vec d;
		arma::mat U;
		arma::mat V;
		arma::svd_econ(U,d,V,X);

		arma::vec d2 = d%d;
		arma::vec dys = d%(U.t()*y);

		for(int iter=0;iter<burnin;iter++){
			hs_one_step_update_big_n(betacoef,lambda, sigma2_eps, tau2,b_lambda,
                            b_tau, mu, dys,  V,  d2, y, X,
                            A2, A2_lambda, a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				hs_one_step_update_big_n(betacoef,lambda, sigma2_eps, tau2,b_lambda,
                             b_tau, mu, dys,  V,  d2, y, X,
                             A2, A2_lambda, a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}
	} else{

		for(int iter=0;iter<burnin;iter++){
			hs_one_step_update_slice_sampler(betacoef, lambda, sigma2_eps, tau2,
                      b_tau, b_lambda, mu,  y,  X,
                      A2, A2_lambda, a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				hs_one_step_update_slice_sampler(betacoef, lambda, sigma2_eps, tau2,
                       b_tau,b_lambda, mu,  y,  X,
                       A2, A2_lambda, a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}

	}

	betacoef = arma::mean(betacoef_list,1);
	lambda = arma::mean(lambda_list,1);
	sigma2_eps = arma::mean(sigma2_eps_list);
	tau2 = arma::mean(tau2_list);

	Rcpp::List post_mean = Rcpp::List::create(Named("mu") = X*betacoef,
                                           Named("betacoef") = betacoef,
                                           Named("lambda") = lambda,
                                           Named("sigma2_eps") = sigma2_eps,
                                           Named("tau2") = tau2);
	Rcpp::List mcmc = Rcpp::List::create(Named("betacoef") = betacoef_list,
                                      Named("lambda") = lambda_list,
                                      Named("sigma2_eps") = sigma2_eps_list,
                                      Named("tau2") = tau2_list);

	double elapsed = timer.toc();
	return Rcpp::List::create(Named("post_mean") = post_mean,
                           Named("mcmc") = mcmc,
                           Named("elapsed") = elapsed);
}

//'@title Fast Bayesian high-dimensional linear regression with horseshoe priors
//'@param y vector of n outcome variables
//'@param X n x p matrix of candidate predictors
//'@param mcmc_sample number of MCMC iterations saved
//'@param burnin number of iterations before start to save
//'@param thinning number of iterations to skip between two saved iterations
//'@param a_sigma shape parameter in the inverse gamma prior of the noise variance
//'@param b_sigma rate parameter in the inverse gamma prior of the noise variance
//'@param A_tau scale parameter in the half Cauchy prior of the global shrinkage parameter
//'@param A_lambda scale parameter in the half Cauchy prior of the local shrinkage parameter
//'@return a list object consisting of two components
//'\describe{
//'\item{post_mean}{a list object of four components for posterior mean statistics}
//'\describe{
//'\item{mu}{a vector of posterior predictive mean of the n training sample}
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{lambda}{a vector of posterior mean of p local shrinkage parameters}
//'\item{sigma2_eps}{posterior mean of the noise variance}
//'\item{b_lambda}{posterior mean of the rate parameter in the prior for local shrinkage parameters}
//'\item{tau2}{posterior mean of the global parameter}
//'}
//'\item{mcmc}{a list object of three components for MCMC samples}
//'\describe{
//'\item{betacoef}{a matrix of MCMC samples of p regression coeficients}
//'\item{lambda}{a matrix of MCMC samples of p local shrinkage parameters}
//'\item{sigma2_eps}{a vector of MCMC samples of the noise variance}
//'\item{b_lambda}{a vector of MCMC samples of the rate parameter in the prior for local shrinkage parameters}
//'\item{tau2}{a vector of MCMC samples of the global shrinkage parameter}
//'}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'set.seed(2022)
//'dat1 <- sim_linear_reg(n=2000,p=200,X_cor=0.9,q=6)
//'res1 <- with(dat1,fast_horseshoe_ss_lm(y,X))
//'dat2 <- sim_linear_reg(n=200,p=2000,X_cor=0.9,q=6)
//'res2 <- with(dat2,fast_horseshoe_ss_lm(y,X))
//'tab <- data.frame(rbind(comp_sparse_SSE(dat1$betacoef,res1$post_mean$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2$post_mean$betacoef)),
//'time=c(res1$elapsed,res2$elapsed))
//'rownames(tab)<-c("n = 2000, p = 200","n = 200, p = 2000")
//'fast_horseshoe_tab <- tab
//'print(fast_horseshoe_tab)
//'@export
// [[Rcpp::export]]
Rcpp::List fast_horseshoe_hd_lm(arma::vec& y, arma::mat& X,
                             int mcmc_sample = 500,
                             int burnin = 500, int thinning = 1,
                             double a_sigma = 0.0, double b_sigma = 0.0,
                             double A_tau = 1, double A_lambda = 1){

	arma::wall_clock timer;
	timer.tic();

	int p = X.n_cols;
	int n = X.n_rows;
	double sigma2_eps = 1;
	if(a_sigma!=0.0){
		sigma2_eps = b_sigma/a_sigma;
	}
	double A2 = A_tau*A_tau;
	double A2_lambda = A_lambda*A_lambda;
	double b_tau = 1;

	double tau2 = 1.0/p;

	arma::vec betacoef;
	arma::vec lambda;
	arma::vec b_lambda;
	lambda.ones(p);
	b_lambda.ones(p);
	arma::vec mu;

	arma::mat betacoef_list;
	arma::mat lambda_list;
	arma::vec sigma2_eps_list;
	arma::vec tau2_list;

	betacoef_list.zeros(p,mcmc_sample);
	lambda_list.zeros(p,mcmc_sample);
	sigma2_eps_list.zeros(mcmc_sample);
	tau2_list.zeros(mcmc_sample);


	if(p<n){
		arma::vec d;
		arma::mat U;
		arma::mat V;
		arma::svd_econ(U,d,V,X);

		arma::vec d2 = d%d;
		arma::vec dys = d%(U.t()*y);

		for(int iter=0;iter<burnin;iter++){
			hs_one_step_update_big_n(betacoef,lambda, sigma2_eps, tau2,b_lambda,
                            b_tau, mu, dys,  V,   d2, y, X,
                            A2, A2_lambda, a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				hs_one_step_update_big_n(betacoef,lambda, sigma2_eps, tau2,b_lambda,
                             b_tau, mu, dys,  V,  d2, y, X,
                             A2, A2_lambda, a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}
	} else{

		for(int iter=0;iter<burnin;iter++){
			hs_one_step_update(betacoef, lambda, sigma2_eps, tau2,
                            b_tau, b_lambda, mu,  y,  X,
                            A2, A2_lambda, a_sigma,  b_sigma, p,  n);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				hs_one_step_update(betacoef, lambda, sigma2_eps, tau2,
                             b_tau,b_lambda, mu,  y,  X,
                             A2, A2_lambda, a_sigma,  b_sigma, p,  n);
			}
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}

	}

	betacoef = arma::mean(betacoef_list,1);
	lambda = arma::mean(lambda_list,1);
	sigma2_eps = arma::mean(sigma2_eps_list);
	tau2 = arma::mean(tau2_list);

	Rcpp::List post_mean = Rcpp::List::create(Named("mu") = X*betacoef,
                                           Named("betacoef") = betacoef,
                                           Named("lambda") = lambda,
                                           Named("sigma2_eps") = sigma2_eps,
                                           Named("tau2") = tau2);
	Rcpp::List mcmc = Rcpp::List::create(Named("betacoef") = betacoef_list,
                                      Named("lambda") = lambda_list,
                                      Named("sigma2_eps") = sigma2_eps_list,
                                      Named("tau2") = tau2_list);

	double elapsed = timer.toc();
	return Rcpp::List::create(Named("post_mean") = post_mean,
                           Named("mcmc") = mcmc,
                           Named("elapsed") = elapsed);
}


//'@title Prediction with fast Bayesian linear regression fitting
//'@param model_fit  output list object of fast Bayesian linear regression fitting (see value of \link{fast_horseshoe_lm} as an example)
//'@param X_test \eqn{n} by \eqn{p} matrix of predictors for the test data
//'@param alpha posterior predictive credible level \eqn{\alpha \in (0,1)}. The default value is \eqn{0.95}.
//'@return a list object consisting of three components
//'\describe{
//'\item{mean}{a vector of \eqn{n} posterior predictive mean values}
//'\item{ucl}{a vector of \eqn{n}  posterior \eqn{\alpha} level upper credible limits}
//'\item{lcl}{a vector of \eqn{n}  posterior \eqn{\alpha} level lower credible limits}
//'\item{median}{a vector of \eqn{n}  posterior predictive median values}
//'\item{sd}{a vector of \eqn{n}  posterior predictive standard deviation values}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'dat <- sim_linear_reg(n=2000,p=200,X_cor=0.9,q=6)
//'train_idx = 1:round(length(dat$y)/2)
//'test_idx = setdiff(1:length(dat$y),train_idx)
//'res <- fast_horseshoe_lm(dat$y[train_idx],dat$X[train_idx,])
//'pred_res <- predict_fast_lm(res,dat$X[test_idx,])
//'plot(dat$y[test_idx,],pred_res$mean,
//'type="p",pch=19,cex=0.5,col="blue",asp=1,xlab="Observations",
//'ylab = "Predictions")
//'abline(0,1)
//'@export
// [[Rcpp::export]]
Rcpp::List predict_fast_lm(Rcpp::List& model_fit, arma::mat& X_test, double alpha = 0.95){

	Rcpp::List mcmc = model_fit["mcmc"];
	arma::mat betacoef = mcmc["betacoef"];
	arma::mat pred_mu = X_test*betacoef;
	double alpha_1 = (1-alpha)*0.5;
	arma::vec pvec = {1.0 - alpha_1,alpha_1};
	arma::vec pred_mean = arma::mean(pred_mu,1);
	arma::mat pred_cls = arma::quantile(pred_mu,pvec,1);
	arma::vec pred_median = arma::median(pred_mu,1);
	arma::vec pred_sd = arma::stddev(pred_mu,0,1);


	Rcpp::List pred = Rcpp::List::create(Named("mean") = pred_mean,
                                      Named("ucl") = pred_cls.col(0),
                                      Named("lcl") = pred_cls.col(1),
                                      Named("median") = pred_median,
                                      Named("sd") = pred_sd);

	return pred;
}

//'@title Prediction with fast Bayesian logistic regression fitting
//'@param model_fit  output list object of fast Bayesian logistic regression fitting (see value of \link{fast_horseshoe_lm} as an example)
//'@param X_test \eqn{n} by \eqn{p} matrix of predictors for the test data
//'@param alpha posterior predictive credible level \eqn{\alpha \in (0,1)}. The default value is \eqn{0.95}.
//'@param cutoff threshold value for posterior predicitve probablity. The default value is 0.5
//'@return a list object consisting of three components
//'\describe{
//'\item{class}{a vector of \eqn{n} predicted class indicators}
//'\item{mean}{a vector of \eqn{n} posterior predictive mean probabilities}
//'\item{ucl}{a vector of \eqn{n}  posterior \eqn{\alpha} level upper credible limits of probabilities}
//'\item{lcl}{a vector of \eqn{n}  posterior \eqn{\alpha} level lower credible limits of probabilities}
//'\item{median}{a vector of \eqn{n}  posterior predictive median probabilities}
//'\item{sd}{a vector of \eqn{n}  standard deviations of posterior predictive probabilities}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'dat <- sim_logit_reg(n=2000,p=200,X_cor=0.9,q=6)
//'train_idx = 1:round(length(dat$y)/2)
//'test_idx = setdiff(1:length(dat$y),train_idx)
//'res <- fast_normal_logit(dat$y[train_idx],dat$X[train_idx,])
//'pred_res <- predict_fast_logit(res,dat$X[test_idx,])
//'plot(dat$prob[test_idx,],pred_res$mean,
//'type="p",pch=19,cex=0.5,col="blue",asp=1,xlab="True Probabilities",
//'ylab = "Predicted Probabilities")
//'abline(0,1)
//'print(comp_class_acc(pred_res$class,dat$y[test_idx]))
//'@export
// [[Rcpp::export]]
Rcpp::List predict_fast_logit(Rcpp::List& model_fit, arma::mat& X_test,
                              double alpha = 0.95, double cutoff = 0.5){

	Rcpp::List mcmc = model_fit["mcmc"];
	arma::mat betacoef = mcmc["betacoef"];
	arma::mat pred_mu = X_test*betacoef;
	arma::mat pred_prob = 1.0/(1.0 + exp(-pred_mu));
	double alpha_1 = (1-alpha)*0.5;
	arma::vec pvec = {1.0 - alpha_1,alpha_1};
	arma::vec pred_mean = arma::mean(pred_prob,1);
	arma::mat pred_cls = arma::quantile(pred_prob,pvec,1);
	arma::vec pred_median = arma::median(pred_prob,1);
	arma::vec pred_sd = arma::stddev(pred_prob,0,1);
	arma::uvec pred_class;
	pred_class.zeros(pred_mean.n_elem);
	pred_class.elem(arma::find(pred_mean>cutoff)).ones();


	Rcpp::List pred = Rcpp::List::create(Named("class") = pred_class,
																			Named("mean") = pred_mean,
                                      Named("ucl") = pred_cls.col(0),
                                      Named("lcl") = pred_cls.col(1),
                                      Named("median") = pred_median,
                                      Named("sd") = pred_sd);

	return pred;
}



void scalar_img_one_step_update(arma::vec& theta, arma::uvec& delta, arma::vec& lambda,
                              double& sigma2_eps, double& tau2,
                              double& b_tau, arma::vec& b_lambda,  arma::vec& betacoef,
                              arma::vec& y, arma::mat& X, arma::mat& Z, arma::mat& Phi, arma::vec& eps,
                              double& A2, double& A2_lambda,
                              double a_sigma, double b_sigma,
                              int p, int n, int L){


	//update delta
	for(int j=0;j<p;j++){
		//update mu_j
		arma::vec xphi_j = X.col(j)*betacoef(j);
		arma::vec eps_j = eps;
		if(delta(j)==1L){
			eps_j -= xphi_j;
		}
		arma::vec eps_j_1 = eps_j - xphi_j;
		double log_prob_1 = -0.5*arma::accu(eps_j_1%eps_j_1)/sigma2_eps;
		double log_prob_0 = -0.5*arma::accu(eps_j%eps_j)/sigma2_eps;
		double prob = 0.0;
		if(log_prob_1>log_prob_0){
			prob = 1.0/(1.0+exp(log_prob_0-log_prob_1));
		} else{
			prob = exp(log_prob_1-log_prob_0);
			prob = prob/(1+prob);
		}
		if(arma::randu<double>() < prob){
			delta(j) = 1L;
			eps = eps_j - xphi_j;
		} else{
			delta(j) = 0L;
			eps = eps_j;
		}
	}
	//update Z
	arma::uvec active_idx = arma::find(delta==1L);
	Z = X.cols(active_idx)*Phi.rows(active_idx);

	//update theta
	arma::vec lambda2 = lambda%lambda;
	double sigma_eps = sqrt(sigma2_eps);
	double tau = sqrt(tau2);
	double inv_tau2 = 1.0/tau2;
	arma::vec alpha_1 = arma::randn<arma::vec>(p)%lambda;
	alpha_1 *= sigma_eps*tau;
	arma::vec alpha_2 = arma::randn<arma::vec>(n);
	alpha_2 *= sigma_eps;
	arma::mat LambdaZt = Z.t();
	for(int i=0;i<n;i++){
		LambdaZt.col(i) %= lambda;
	}
	arma::mat ZZt = arma::eye(n,n);
	ZZt += tau2*LambdaZt.t()*LambdaZt;
	arma::vec theta_s = arma::solve(ZZt, y - Z*alpha_1 - alpha_2,arma::solve_opts::fast);
	theta = alpha_1 + tau2*lambda2%(Z.t()*theta_s);
	//update betacoef
	betacoef = Phi*theta;
	//update eps
	eps = y - Z*theta;


	//update lambda
	arma::vec theta2 = theta%theta;
	arma::vec inv_lambda2 = arma::randg<arma::vec>(p,distr_param(1.0,1.0));
	inv_lambda2 /= b_lambda + 0.5*theta2/tau2/sigma2_eps;
	b_lambda = randg<arma::vec>(p,distr_param(1.0, 1.0));
	b_lambda /= 1.0/A2_lambda+inv_lambda2;
	lambda = sqrt(1.0/inv_lambda2);

	//update tau2, sigma2_eps, b_tau and b_lambda
	double sum_eps2 = arma::accu(eps%eps);
	double sum_theta2_inv_lambda2 = arma::accu(theta2%inv_lambda2);
	inv_tau2 = randg<double>(distr_param((1.0+L)/2.0,1.0/(b_tau+0.5*sum_theta2_inv_lambda2/sigma2_eps)));
	b_tau = randg<double>(distr_param(1.0,1.0/(1.0/A2 + inv_tau2)));
	tau2 = 1.0/inv_tau2;
	double inv_sigma2_eps = arma::randg<double>(distr_param(a_sigma+(L+n)/2, 1.0/(b_sigma+0.5*sum_theta2_inv_lambda2*inv_tau2+0.5*sum_eps2)));
	sigma2_eps = 1.0/inv_sigma2_eps;
}

/*
//'@title Fast Bayesian Scalar-on-Image linear regression with Gaussian process priors
//'@param y vector of n outcome variables
//'@param X n x p matrix of candidate predictors
//'@param Phi p x L matrix of basis function
//'@param mcmc_sample number of MCMC iterations saved
//'@param burnin number of iterations before start to save
//'@param thinning number of iterations to skip between two saved iterations
//'@param a_sigma shape parameter in the inverse gamma prior of the noise variance
//'@param b_sigma rate parameter in the inverse gamma prior of the noise variance
//'@param A_tau scale parameter in the half Cauchy prior of the global shrinkage parameter
//'@param A_lambda scale parameter in the half Cauchy prior of the local shrinkage parameter
//'@return a list object consisting of two components
//'\describe{
//'\item{post_mean}{a list object of four components for posterior mean statistics}
//'\describe{
//'\item{mu}{a vector of posterior predictive mean of the n training sample}
//'\item{betacoef}{a vector of posterior mean of p regression coeficients}
//'\item{lambda}{a vector of posterior mean of p local shrinkage parameters}
//'\item{sigma2_eps}{posterior mean of the noise variance}
//'\item{tau2}{posterior mean of the global parameter}
//'}
//'\item{mcmc}{a list object of three components for MCMC samples}
//'\describe{
//'\item{betacoef}{a matrix of MCMC samples of p regression coeficients}
//'\item{lambda}{a matrix of MCMC samples of p local shrinkage parameters}
//'\item{sigma2_eps}{a vector of MCMC samples of the noise variance}
//'\item{tau2}{a vector of MCMC samples of the global shrinkage parameter}
//'}
//'}
//'@author Jian Kang <jiankang@umich.edu>
//'@examples
//'set.seed(2022)
//'dat1 <- sim_linear_reg(n=2000,p=200,X_cor=0.9,q=6)
//'res1 <- with(dat1,fast_horseshoe_lm(y,X))
//'dat2 <- sim_linear_reg(n=200,p=2000,X_cor=0.9,q=6)
//'res2 <- with(dat2,fast_horseshoe_lm(y,X))
//'tab <- data.frame(rbind(comp_sparse_SSE(dat1$betacoef,res1$post_mean$betacoef),
//'comp_sparse_SSE(dat2$betacoef,res2$post_mean$betacoef)),
//'time=c(res1$elapsed,res2$elapsed))
//'rownames(tab)<-c("n = 2000, p = 200","n = 200, p = 2000")
//'fast_horseshoe_tab <- tab
//'print(fast_horseshoe_tab)
//'@export
// [[Rcpp::export]]
Rcpp::List fast_scalar_img_lm(arma::vec& y_, arma::mat& X_, arma::mat>& Phi,
                             int mcmc_sample = 500,
                             int burnin = 500, int thinning = 1,
                             double a_sigma = 0.0, double b_sigma = 0.0,
                             double A_tau = 1, double A_lambda = 1){

	arma::wall_clock timer;
	timer.tic();

	arma::vec y;
	arma::mat X;

	if(p<n){
		arma::vec d;
		arma::mat U;
		arma::mat V;
		arma::svd_econ(U,d,V,X_);
		y = U.t()*y_;
		X = U.t()*X_;
	} else{
		y = y_;
		X = X_;
	}

	int p = X.n_cols;
	int n = X.n_rows;
	int L = Phi.n_cols;

	double sigma2_eps = 1;
	if(a_sigma!=0.0){
		sigma2_eps = b_sigma/a_sigma;
	}
	double A2 = A_tau*A_tau;
	double A2_lambda = A_lambda*A_lambda;
	double b_tau = 1;

	double tau2 = 1;
	arma::vec d2 = d%d;


	arma::vec theta;
	arma::uvec delta;
	arma::vec betacoef;
	arma::vec lambda;
	arma::vec b_lambda;
	arma::vec eps;
	arma::mat Z = X*Phi;

	theta.zeros(L);
	lambda.ones(L);
	b_lambda.ones(L);
	delta.ones(p);
	betacoef.zeros(p);
	eps.zeros(n);


	arma::mat theta_list;
	arma::umat delta_list;
	arma::mat betacoef_list;
	arma::mat lambda_list;
	arma::vec sigma2_eps_list;
	arma::vec tau2_list;


	delta_list.zeros(p,mcmc_sample);
	betacoef_list.zeros(p,mcmc_sample);
	theta_list.zeros(L,mcmc_sample);
	lambda_list.zeros(L,mcmc_sample);
	sigma2_eps_list.zeros(mcmc_sample);
	tau2_list.zeros(mcmc_sample);


		arma::mat XtX_inv = V*diagmat(1.0/d2)*V.t();
		arma::vec Xty = X.t()*y;
		for(int iter=0;iter<burnin;iter++){
			scalar_img_one_step_update(theta, delta, lambda,
                              sigma2_eps, tau2,
                              b_tau,  b_lambda,  betacoef,
                              y,  X,  Z,  Phi, eps, A2,  A2_lambda,
                              a_sigma, b_sigma, p, n, L);
		}
		for(int iter=0;iter<mcmc_sample;iter++){
			for(int j=0;j<thinning;j++){
				scalar_img_one_step_update(theta, delta, lambda,
                               sigma2_eps, tau2,
                                b_tau,  b_lambda,  betacoef,
                                y,  X,  Z,  Phi, eps, A2,  A2_lambda,
                                a_sigma, b_sigma, p, n, L);
			}
			theta_list.col(iter) = theta;
			delta_list.col(iter) = delta;
			betacoef_list.col(iter) = betacoef;
			lambda_list.col(iter) = lambda;
			sigma2_eps_list(iter) = sigma2_eps;
			tau2_list(iter) = tau2;
		}


	theta = arma::mean(theta_list,1);
	arma::vec delta_prob = arma::mean(delta_list*1.0,1);
	betacoef = arma::mean(betacoef_list,1);
	lambda = arma::mean(lambda_list,1);
	sigma2_eps = arma::mean(sigma2_eps_list);
	tau2 = arma::mean(tau2_list);

	Rcpp::List post_mean = Rcpp::List::create(Named("mu") = X_*betacoef,
                                           Named("betacoef") = betacoef,
                                           Named("theta") = theta,
                                           Named("delta_prob") = delta_prob,
                                           Named("lambda") = lambda,
                                           Named("sigma2_eps") = sigma2_eps,
                                           Named("tau2") = tau2);
	Rcpp::List mcmc = Rcpp::List::create(Named("betacoef") = betacoef_list,
                                      Named("lambda") = lambda_list,
                                      Named("sigma2_eps") = sigma2_eps_list,
                                      Named("tau2") = tau2_list);

	double elapsed = timer.toc();
	return Rcpp::List::create(Named("post_mean") = post_mean,
                           Named("mcmc") = mcmc,
                           Named("elapsed") = elapsed);
}*/
