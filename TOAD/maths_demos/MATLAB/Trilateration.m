function ret = Trilateration(args,n)
%%cellArray argument format:
%   args{1,i} = position vector of reference point i, p_i
%   args{2,i} = distance from reference point i, r_i

% n is number of dimensions (2 or 3)

%   Derivation: Y. Zhou, "An Efficient Least-Squares Trilateration 
%   Algorithm for Mobile Robot Localization"

    %% Check input format
    if ~iscell(args)
        error('input must be cell array')
    end
    
    arg_dims = size(args);
    N = arg_dims(2);
    if arg_dims(1) ~= 2
        error('input must be 2 x n cell array')
    end
	if N < n % >=n reference points
		error('Need at least n reference points')
    end
    
    if ~isscalar(n) || (n ~=2 && n ~=3)
        error('Invalid number of spatial dimensions')
    end
    
    for i = 1:N
        size_p_i = size(args{1,i});
        if size_p_i(1) ~= n || size_p_i(2) ~= 1
            % not an n element column vector
            error('Invalid argument type, field {1,%d}',i)
        end
        if  ~isscalar(args{2,i})
            error('Invalid argument type, field {2,%d}',i)
        end
    end
    %end input format check
    
    fprintf('Number of reference points: N = %d\n',N);
    %% Step 1: Calculate useful variables
    a = zeros(n,1);
    B = zeros(n,1);
    c = zeros(n,1);
    for i = 1:N
        p_i = args{1,i};
        r_i = args{2,i};
        
        a = a + (p_i * transpose(p_i) * p_i - (r_i^2) * p_i);
        B = B + ((-2) * p_i * transpose(p_i) - (transpose(p_i) * p_i) * eye(n) + (r_i^2) * eye(n));
        c = c + p_i;
    end
    
    a = a / N
    B = B / N
    c = c / N
    
    f = a + B*c + 2*c*transpose(c)*c
    fprime = zeros(n-1,1);
    H = zeros(n,n);
    Hprime = zeros(n-1,n);
    for i = 1:N
        p_i = args{1,i};
        H = H + (p_i * transpose(p_i) + 2 * c * transpose(c));
    end
    H = H * (-2) / N
    
    for i=1:n-1
        fprime(i) = f(i) - f(n);
        Hprime(i,:) = H(i,:) - H(n,:);
    end
    % show values
    fprime
    Hprime
    
    
    [Q,U] = qr(Hprime)
    %end of step 1
    
    %% Step 2: Calculate qtq
    qtq = 0;
    for i = 1:N
        p_i = args{1,i};
        r_i = args{2,i};
        qtq = qtq + (r_i^2 + transpose(c)*c - transpose(p_i)*p_i);
    end
    qtq = qtq / N
    %end of step 2
    
    %% Steps 3 to 5: dependent on n
    v = transpose(Q)*fprime;
    q = zeros(n,2)
    if n == 2
        %Step 3
        %[poly_v + poly_u*q(2)]^2 + q(2)^2 = qtq
        poly_v = v(1)/U(1,1);
        poly_u = U(1,2)/U(1,1);
        q(2,:) = roots([poly_u^2 + 1, 2*poly_u*poly_v, poly_v^2 - qtq])
        
        %Step 4
        q(1,:) = 0 - poly_v - poly_u * q(2,:);
    elseif n == 3
        %Step 3
        %[a+b*q(3)]^2 + [c+d*q(3)]^2+q(3)^2 = qtq
        poly_a = (U(1,2)*v(2)) / (U(1,1)*U(2,2)) - v(1)/U(1,1);
        poly_b = (U(1,2)*U(2,3)) / (U(1,1)*U(2,2)) - U(1,3)/U(1,1);
        poly_c = v(2)/U(2,2);
        poly_d = U(2,3)/U(2,2);
        q(3,:) = roots([1 + poly_b^2 + poly_d^2,...
            2*(poly_a * poly_b + poly_c * poly_d),...
            poly_a^2 + poly_c^2 - qtq])
        
        %Step 4
        q(1,:) = poly_a + poly_b * q(3,:);
        q(2,:) = 0 - poly_c - poly_d * q(3,:);
    else 
        error('n should be 2 or 3')
    end
    
    %Step 5
    p0 = q + [c,c]
    %end of section 3-5
    
    %% Section 6 to 7: Select p0
    %Assume that correct solution has a positive z component
    if p0(3,1) > 0
        ret = p0(:,1)
    elseif p0(3,2) >= 0
        ret = p0(:,2)
    else
        error('Both position estimates on wrong side of plane')
    end
end