%% Figure: Static RAM footprint comparison (TASA vs EcoTrack)

algos = categorical({'TASA', 'EcoTrack'});
algos = reordercats(algos, {'TASA', 'EcoTrack'});  % preserve this order
ram = [588, 40];

figure('Position', [100 100 500 350]);
b = bar(algos, ram, 0.5);
b.FaceColor = 'flat';
b.CData(1,:) = [0.18 0.33 0.59];   % TASA - dark blue
b.CData(2,:) = [0.65 0.65 0.65];   % EcoTrack - grey

ylabel('Static RAM (bytes)');
title('Static RAM Footprint');
ylim([0 680]);
box off;

% Add value labels above each bar
for i = 1:length(ram)
    text(i, ram(i) + 15, num2str(ram(i)), ...
        'HorizontalAlignment', 'center', 'FontSize', 10);
end

set(gca, 'FontName', 'Times New Roman', 'FontSize', 11);
exportgraphics(gcf, 'fig_ram_comparison.pdf', 'ContentType', 'vector');
exportgraphics(gcf, 'fig_ram_comparison.png', 'Resolution', 300);
