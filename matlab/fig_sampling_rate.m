%% Figure: Average sampling rate vs fixed-rate baseline (log scale)

algos = categorical({'Fixed-rate (200 Hz)', 'TASA', 'EcoTrack (repurposed)'});
algos = reordercats(algos, {'Fixed-rate (200 Hz)', 'TASA', 'EcoTrack (repurposed)'});
rates = [200, 66.8, 0.25];

figure('Position', [100 100 550 400]);
b = bar(algos, rates, 0.5);
b.FaceColor = 'flat';
b.CData(1,:) = [0.65 0.65 0.65];   % Fixed-rate - grey
b.CData(2,:) = [0.18 0.33 0.59];   % TASA - blue
b.CData(3,:) = [0.75 0.0  0.0 ];   % EcoTrack - red

set(gca, 'YScale', 'log');
ylim([0.1 600]);
ylabel('Average Sampling Rate (Hz, log scale)');
title('Average Sampling Rate vs. Fixed-Rate Baseline');
box off;

for i = 1:length(rates)
    text(i, rates(i) * 1.5, sprintf('%.2f Hz', rates(i)), ...
        'HorizontalAlignment', 'center', 'FontSize', 10);
end

set(gca, 'FontName', 'Times New Roman', 'FontSize', 11);
exportgraphics(gcf, 'fig_sampling_rate.pdf', 'ContentType', 'vector');
exportgraphics(gcf, 'fig_sampling_rate.png', 'Resolution', 300);
