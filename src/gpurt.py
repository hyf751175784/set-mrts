import math as math

g = 2
m = 2048


def get_gpurt(L,H,h,h_i,l_i,w):
  # sum = math.sum(w)
  return l_i+(L*(g*m-H)-h_i*l_i+w)/(g*(m-H+h))


def get_term(L,H,h,h_i,l_i):
  # sum = math.sum(w)
  return l_i+(L*(g*m-H)-h_i*l_i)/(g*(m-H+h))


