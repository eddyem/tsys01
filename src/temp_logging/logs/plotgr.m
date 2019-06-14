function plotgr(nm)
    h = gcf();
    TL = get(gca, 'ticklength');
    set(gca, 'ticklength', TL*1.5);
    set(gca, 'linewidth', 3);
    H = 8; W = 12;
    lg = findobj(h, "type","axes", "Tag","legend");
    set(lg, "FontSize", 16);
    set(gca, 'fontsize', 16);
    set(get(gca(), "xlabel"), 'fontsize', 16);
    set(get(gca(), "ylabel"), 'fontsize', 16);
    set(h,'PaperUnits','inches')
    set(h,'PaperOrientation','landscape');
    set(h,'PaperSize',[W,H]);
    set(h,'PaperPosition',[0,0,W,H]);
    print(h, '-dpdf', sprintf("%s.pdf",  nm));
    print(h, '-dpng', sprintf("%s.png", nm));
endfunction
