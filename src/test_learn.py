import numpy as np
from manet.learn import SampleDMN, one_hot
from tqdm import tqdm

# Discrete Markov Network
# Input symbols and hidden states are from a finite set


if __name__ == "__main__":

    np.random.seed(2)

    X = np.array([0,1,2,3,0,0,1,1,2,3])
    Y = np.array([0,1,1,0,2,2,1,0,0,1])
    nY = 3
    nX = 4

    length = X.size
    E = np.concatenate(( np.arange(0,length-1).reshape((1,length-1)),
                         np.arange(1,length).reshape((1,length-1)) ),axis=0)

    sample = SampleDMN(nY,one_hot(nX,X),Y,E)
    sample.alpha12 = 2*np.random.normal(0,1,(sample.n_y,sample.n_edges))
    sample.alpha21 = 2*np.random.normal(0,1,(sample.n_y,sample.n_edges))

    W = np.random.normal(0,1,sample.n_params )

    loss = sample.eval_margin_rescaling_loss( W )

    G = sample.get_pair_scores(W)
    Q = sample.get_unary_scores(W)
    print( G.shape)
    print( Q.shape)

    print(loss)

#    print(sample.grad_weights)
#    print(sample.grad_alpha12)
#    print(sample.grad_alpha21)

