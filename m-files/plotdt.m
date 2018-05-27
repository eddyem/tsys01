function plotdt(T, idxsch)
    idxsch *= 2;
    difrns = T(:,idxsch-1)-T(:,idxsch);
    D = median(difrns);
    sgn = '+';
    if(D < 0) sgn = '-'; endif
    sigma = std(difrns);
    printf("T_1(%d) = T_0(%d) %c%.4f +- %.4f\n", idxsch, idxsch, sgn, abs(D), sigma);
    plot(T(:,1), medfilt1(D-difrns, 5));
    %plot(T(:,1), T(:,idxsch)-T(:,idxsch-1)+D);
    xlabel("T, ^\\circ{}C");
    ylabel(sprintf("T_1-T_0%c%.4f", sgn, abs(D)));
    title(sprintf("Temperature difference for pair %d (\\sigma=%.4f)", idxsch/2, sigma));
endfunction
