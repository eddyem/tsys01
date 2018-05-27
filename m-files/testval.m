function v = testval()
    K = [34100, 22617, 15996, 7338, 5714];
    printf("RAW\t\tdouble\tint\n");
    for raw = 8000000:100000:10000000;
        ii = getint(K, raw);
        s = sign(ii);
        ii = abs(ii);
        f = floor(ii/100);
        printf("%10d\t%.2f\t%d.%02d\n", raw, getdouble(K, raw), f*s, ii-100*f);
    endfor
endfunction

function T = getdouble(K, raw)
    val = raw/256.;
    T = K(1)*(-1.5e-2) + 0.1e-5*val*(K(2) + 1e-5*val*(K(3)*(-2) + 1e-5*val*(4*K(4) + 1e-5*val*(-2)*K(5))));
endfunction

function T = getint(K, raw)
    val = int64(raw);
    mul = int64([0, 1, -2, 4, -2]);
    tmp = int64(0);
    for j = 5:-1:2
        tmp /= 100000;
        tmp += mul(j) * K(j);
        tmp *= val;
        tmp /= 256;
    endfor
    T = double(tmp*1e-4 + (-1.5)*K(1));
endfunction
