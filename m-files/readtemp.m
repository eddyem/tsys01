function [tm T goodrecs diffs] = readtemp(name)
% return arrays of unix time & temperatures from file `name`
% substitute missed values by interpolation
    T = dlmread(name);
    nrecs = size(T, 1);
    nsensors = size(T, 2) - 1;
    if(nrecs < 2 || nsensors < 2) return; endif
    tm = T(:, 1);
    tm -= tm(1);
    T(:,1) = [];
    goodrecs = [];
    % interpolate missing values
    for N = 1:nsensors
        term = T(:, N);
        badidxs = find(term < -275.);
        bads = size(badidxs, 1);
        if(bads > nrecs/2)
            printf("%d: all (or almost all) data records are bad\n", N);
            continue;
        elseif(bads) % some records are bad, try to make interpolation
            printf("%d: %d bad records\n", N, bads);
            tmx = tm; Tx = term;
            tmx(badidxs) = [];
            Tx(badidxs) = [];
            T(:, N) = interp1(tmx, Tx, tm, "spline");
        endif
        goodrecs = [goodrecs N];
    endfor
    % remove strings with NANs
    ii = find(isnan(T));
    s = size(T, 1);
    idxs = ii - s*floor((ii-1)/s);
    T(idxs, :) = [];
    tm(idxs) = [];
    diffs = T - median(T,2);
    % now remove bad records
    for N = goodrecs
        term = abs(diffs(:, N));
        badidxs = find(term > 3.);
        bads = size(badidxs, 1);
        if(bads)
            tmx = tm; Tx = T(:, N);
            tmx(badidxs) = [];
            Tx(badidxs) = [];
            T(:, N) = interp1(tmx, Tx, tm);
        endif
    endfor
    diffs = T - median(T,2);
endfunction
