"""Trilateration maths functions.
"""

def ecef_transform():

def speedy_trilat(p_i, r_i, guess = False):
    """Returns position estimate from ground station locations and distances between them and tracked objects.

    Derivation: Y. Zhou, "An Efficient Least-Squares Trilateration Algorithm for Mobile Robot Localization".
    Matlab implementations in TOAD documentation.

    Args:
        p_i ( (n,N) numpy array): position vectors of reference points
        r_i ( (n,) numpy array): distances from each reference point
        guess (boolean): set true to assume that z=0 if no intersection
                         point can be found

    Returns:
        Position of tracked object as an n element column vector (numpy array)

    Raises:
        Asserts n (number of dimensions) is 2 or 3
        ValueError if no real solution to quadratic equation in step 3 exists(circles/spheres do not intersect)
            (and guess is set to false)
    """

    n = np.shape(p_i)[0]
    N = np.shape(p_i)[1]
    assert ( n==2 or n==3 ) , "Position vectors should have two or three dimensions"
    assert ( np.shape(r_i)==(N,) ), "Number of ground stations described by p_i and r_i do not match"

    # Step 1: Calculate useful variables
    N = p_i.shape[1]
    pt_p = p_i * p_i
    pt_ps = np.sum(pt_p, axis=0)
    a = p_i * pt_ps[np.newaxis,:] - p_i * (r_i*r_i)[np.newaxis,:]
    a = np.sum(a, axis=1)

    B = -2 * np.dot(p_i, np.transpose(p_i)) + np.sum(r_i*r_i - pt_ps) * np.identity(n)

    c = np.sum(p_i, axis=1)

    a = a/N
    B = B/N
    c = c/N

    a_mat = a[:,np.newaxis]  # Single dimension matrix
    c_mat = c[:,np.newaxis]  # Single dimension matrix

    f = a_mat + np.dot(B,c_mat) + 2*np.dot( np.dot( c_mat, np.transpose(c_mat) ) , c_mat )
    H = ( np.dot(p_i, np.transpose(p_i)) + 2*N*np.dot(c_mat, np.transpose(c_mat)) ) * (-2/N)

    fprime = f - f[n-1]
    fprime = np.delete(fprime, (n-1), axis=0)
    Hprime = H - H[n-1,:][np.newaxis,:]
    Hprime = np.delete(Hprime, (n-1), axis=0)

    Q,U = np.linalg.qr(Hprime)

    # Step 2: Calculate qtq

    qtq = r_i*r_i + np.dot(np.transpose(c_mat),c_mat) - pt_ps
    qtq = np.sum(qtq)/N

    # Steps 3 to 5 - dependent on n
    # Note about complex roots:
    # Quadratic equation has no real roots if spheres/circles do not intersect
    v = np.dot( np.transpose(Q), fprime)
    q = np.zeros(shape=(n,2))
    if n==2:
        # Step 3:
        # Polynomial form [poly_v + poly_u*q(1)]^2 + q(1)^2 = qtq
        poly_v = v[0]/U[0,0]
        poly_u = U[0,1]/U[0,0]

        roots = np.roots(np.array([poly_u*poly_u + 1, 2*poly_u*poly_v, poly_v*poly_v - qtq]))
        if np.iscomplex(roots[0]):
            if guess:
                q[1,] = np.zeros(2)
            else:
                raise ValueError("No real solution to quadratic equation for y coordinate")
        else:
            q[1,] = roots

        # Step 4:
        q[0,:] = -poly_v - poly_u * q[1,:]
    elif n == 3:
        # Step 3:
        poly_a = U[0,1] * v[1] / (U[0,0]*U[1,1]) - v[0]/U[0,0]
        poly_b = U[0,1]*U[1,2] / (U[0,0]*U[1,1]) - U[0,2]/U[0,0]
        poly_c = v[1]/U[1,1]
        poly_d = U[1,2]/U[1,1]
        roots = np.roots(np.array([1 + poly_b**2 + poly_d**2,
            2*(poly_a * poly_b + poly_c * poly_d),
            poly_a**2 + poly_c**2 - qtq]))

        if np.iscomplex(roots[0]):
            if guess:
                q[2,:] = np.zeros(2)
            else:
                raise ValueError("No real solution to quadratic equation for z coordinate")
        else:
            q[2,:] = roots
        # Step 4:
        q[0,:] = poly_a + poly_b * q[2,:]
        q[1,:] = -poly_c - poly_d * q[2,:]

    else:
        raise ValueError("Number of spatial dimensions in input matrices should be 2 or 3")

    # Step 5:
    p0 = q + c[:,np.newaxis]

    # Steps 6 to 7: Select p0

    # Assume that correct solution has positive z component
    # i.e. above ground if xyz axes oriented with plane of ground

    if p0[2,0] > 0:
        pos = p0[:,0]
    elif p0[2,1] >= 0:
        pos = p0[:,1]
    else:
        raise ValueError("Both position estimates on wrong side of plane (negative z component)")

    return pos
