function plotpair(T, idxsch1, idxsch2)
    i1 = 2*idxsch1-1;
    i2 = 2*idxsch2-1;
    difrns = T(:,i1)-T(:,i2);
    D = median(difrns);
    sigma = std(difrns);
    sgn = '+';
    if(D < 0) sgn = '-'; endif
    printf("T_0(%d) = T_0(%d) %c%.4f +- %.4f\n", idxsch2, idxsch1, sgn, abs(D), sigma);
    plot(T(:,1), medfilt1(D-difrns, 5));
    xlabel("T, ^\\circ{}C");
    ylabel(sprintf("T(%d_0) - T(%d_0) %c %.4f", idxsch2, idxsch1, sgn, abs(D)));
    title(sprintf("Temperature difference for sensors %d_0 and %d_0 (\\sigma=%.4f)", idxsch2, idxsch1, sigma));
endfunction
