"""
main.py — Entry point for a strat-optimizer run

Pipeline:
  1. Load the JSON config               → RunConfig
  2. Download historical prices         → numpy array
  3. Generate a grid of parameter combinations (Cartesian product,
     filtered by strategy constraints)
  4. Call the C backtest engine on the TRAINING period
  5. Select the best combination by maximum Sharpe ratio
  6. Re-run the C engine on the TEST period (walk-forward validation)
  7. Plot:
       - heatmap / scatter of performance vs parameters
       - equity curve of the best combination on the test set

All data flows in-memory between Python and C via ctypes — no temp
files, no subprocess, no serialisation.
"""

from .prices import download_prices
from .plotting import plot
from .equity_curve import show_equity_curve
from .strategies import Strategy
from .config import load_config
from .parameters import generate_parameter_combinations
from .backtesting import run_backtesting_engine
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich import box
import sys
import time
import numpy as np

console = Console()


def main():

    t0 = time.perf_counter()

    # ── header ─────────────────────────────────────────────────────
    console.print()
    console.print(
        Panel.fit(
            "[bold bright_white]Strat‑Optimizer[/]  ·  "
            "[dim]Grid‑search backtesting engine[/]",
            border_style="bright_blue",
        )
    )

    # ═══════════════════════════════════════════════════════════════
    # 1. LOAD CONFIG
    # ═══════════════════════════════════════════════════════════════
    t1 = time.perf_counter()
    run = load_config()
    t_config = time.perf_counter() - t1

    asset_label = "Crypto/Forex (365 d/y)" if run.asset.trading_days == 365 \
                  else "Equity (252 d/y)"

    config_table = Table(box=box.SIMPLE_HEAVY, show_header=False,
                         border_style="dim")
    config_table.add_column(style="dim", width=22)
    config_table.add_column(style="bright_white")
    config_table.add_row("Config file",        "[bold]{cfg}[/]"
                          .format(cfg=run._config_path))
    config_table.add_row("Strategy",            "[bold bright_cyan]{s}[/]"
                          .format(s=run.strategy.name))
    config_table.add_row("Asset",              "{t}  [dim]({a})[/]"
                          .format(t=run.asset.ticker, a=asset_label))
    config_table.add_row("Backtest length",     "{d:,} trading days"
                          .format(d=run.backtest_length))
    config_table.add_row("Train / test split",  "{t:.0%} / {v:.0%}"
                          .format(t=1.0 - run.test_size, v=run.test_size))
    config_table.add_row("Lookback",            "{lb} days"
                          .format(lb=run.lookback))

    console.print(Panel(config_table, title="[bold]Configuration",
                        border_style="bright_blue"))

    # ═══════════════════════════════════════════════════════════════
    # 2. DOWNLOAD PRICES
    # ═══════════════════════════════════════════════════════════════
    console.print()
    console.print("  [bold]Downloading prices[/] … ", end="")
    t2 = time.perf_counter()
    number_of_prices, prices, first_date, last_date = download_prices(run)
    t_download = time.perf_counter() - t2

    date_fmt = "%Y-%m-%d"
    console.print("[green]done[/]  [dim]({:.1f}s)[/]".format(t_download))
    console.print(
        "    [dim]{} → {}  │  {} prices  │  {} usable trading days[/]"
        .format(
            first_date.strftime(date_fmt),
            last_date.strftime(date_fmt),
            len(prices),
            number_of_prices - run.lookback,
        )
    )

    # ═══════════════════════════════════════════════════════════════
    # 3. GENERATE PARAMETER GRID
    # ═══════════════════════════════════════════════════════════════
    t3 = time.perf_counter()
    number_of_combinations, parameter_combos, grid_summary = \
        generate_parameter_combinations(run)
    t_grid = time.perf_counter() - t3

    grid_table = Table(box=box.SIMPLE_HEAVY, show_header=False,
                       border_style="dim")
    grid_table.add_column(style="dim", width=28)
    grid_table.add_column(style="bright_white")

    for name, rng_lo, rng_hi, step in grid_summary.free_params:
        n_vals = len(np.arange(rng_lo, rng_hi, step))
        grid_table.add_row(
            name,
            "[{lo} … {hi}]  step={st}  →  {n} values"
            .format(lo=rng_lo, hi=rng_hi, st=step, n=n_vals)
        )
    for name, val in grid_summary.fixed_params:
        grid_table.add_row(
            name + " [dim](fixed)[/]",
            "{}".format(val)
        )

    combo_str = (
        "[bold bright_white]{valid:,}[/] valid combinations"
        .format(valid=grid_summary.valid_combinations)
    )
    if grid_summary.filtered_out > 0:
        combo_str += (
            "  [dim]({:,} total, {:,} pruned by constraints)[/]"
            .format(grid_summary.total_combinations,
                    grid_summary.filtered_out)
        )

    grid_table.add_row("", "")
    grid_table.add_row("[bold]Combinations[/]", combo_str)

    console.print(Panel(grid_table, title="[bold]Parameter Grid",
                        border_style="bright_blue"))

    # ═══════════════════════════════════════════════════════════════
    # 4. TRAIN (in‑sample)
    # ═══════════════════════════════════════════════════════════════
    training_days = int(
        (number_of_prices - run.lookback) * (1.0 - run.test_size)
    )

    console.print()
    console.print(
        "  [bold]Training[/] on {:,} days with {:,} combinations … "
        .format(training_days, number_of_combinations),
        end="", highlight=False,
    )

    t4 = time.perf_counter()
    sys.stdout.flush()
    performances, _equity = run_backtesting_engine(
        run,
        number_of_prices,
        prices,
        combinations=parameter_combos,
    )
    t_train = time.perf_counter() - t4

    total_days_simulated = number_of_combinations * training_days
    console.print(
        "\r  [bold]Training[/] on {td:,} days with {nc:,} combinations"
        " … [green]done[/]  [dim]({tt:.1f}s, {sim:,} days simulated,"
        " {:,.0f} d/s)[/]"
        .format(td=training_days, nc=number_of_combinations,
                tt=t_train, sim=total_days_simulated,
                total_days_simulated / t_train if t_train > 0 else 0))

    # select the combination with the highest Sharpe ratio
    best_performance = performances[0]
    best_combination  = parameter_combos[0]

    for p in range(number_of_combinations):
        perf = performances[p]
        if perf.sharpe_ratio > best_performance.sharpe_ratio:
            best_performance = perf
            best_combination  = parameter_combos[p]

    # ═══════════════════════════════════════════════════════════════
    # 5. TEST (out‑of‑sample / walk‑forward)
    # ═══════════════════════════════════════════════════════════════
    testing_days = int(
        (number_of_prices - run.lookback) * run.test_size
    )

    console.print(
        "  [bold]Testing[/]  on {:,} days (walk‑forward) … "
        .format(testing_days),
        end="", highlight=False,
    )

    t5 = time.perf_counter()
    test_perfs, equity_curve = run_backtesting_engine(
        run,
        number_of_prices,
        prices,
        combinations=[best_combination],
        test_mode=True,
    )
    t_test = time.perf_counter() - t5
    test_performance = test_perfs[0]

    console.print("[green]done[/]  [dim]({:.1f}s)[/]".format(t_test))

    # ═══════════════════════════════════════════════════════════════
    # 6. RESULTS
    # ═══════════════════════════════════════════════════════════════
    console.print()

    # best parameters table
    param_table = Table(box=box.SIMPLE_HEAVY, show_header=False,
                        border_style="dim")
    param_table.add_column(style="dim", width=24)
    param_table.add_column(style="bright_white")
    for p in range(run.strategy.number_of_parameters):
        param_table.add_row(
            run.strategy.parameters[p].name,
            "{}".format(best_combination[p]),
        )
    console.print(Panel(param_table, title="[bold]Best Parameters",
                        border_style="bright_green"))

    # ---- compute buy & hold benchmark -----------------------------
    train_prices = prices[run.lookback : run.lookback + training_days]
    test_prices  = prices[run.lookback + training_days : number_of_prices]

    def _bh(px):
        """Buy & hold: annual profit + Sharpe for a price slice."""
        n = len(px)
        if n < 2:
            return 0.0, 0.0
        profit = float((px[-1] - px[0]) / px[0])
        ann = (1.0 + profit) ** (run.asset.trading_days / n) - 1.0
        rets = (px[1:] - px[:-1]) / px[:-1]
        mu = float(np.mean(rets))
        sd = float(np.std(rets))
        sh = (mu / sd) * np.sqrt(run.asset.trading_days) if sd > 0 else 0.0
        return sh, ann

    bh_train_sh, bh_train_ap = _bh(train_prices)
    bh_test_sh,  bh_test_ap  = _bh(test_prices)

    # performance comparison table
    perf_table = Table(box=box.SIMPLE_HEAVY,
                       border_style="dim",
                       title_style="bold",
                       header_style="bold dim")
    perf_table.add_column("", style="dim", width=10)
    perf_table.add_column("Sharpe Ratio", justify="right", width=13)
    perf_table.add_column("Annual Profit", justify="right", width=13)
    perf_table.add_column("B&H Sharpe", justify="right", width=13)
    perf_table.add_column("B&H Profit", justify="right", width=13)

    perf_table.add_row(
        "[bold]Training[/]",

        "[bold bright_cyan]{:.4f}[/]"
        .format(best_performance.sharpe_ratio),
        "{:+.2%}".format(best_performance.annual_profit),
        "[dim]{:.4f}[/]".format(bh_train_sh),
        "[dim]{:+.2%}[/]".format(bh_train_ap),
    )
    perf_table.add_row(
        "[bold]Testing[/]",

        ("[bold bright_green]{:.4f}[/]"
         if test_performance.sharpe_ratio > 0
         else "[bold red]{:.4f}[/]")
        .format(test_performance.sharpe_ratio),
        ("[bright_green]{:+.2%}[/]"
         if test_performance.annual_profit > 0
         else "[red]{:+.2%}[/]")
        .format(test_performance.annual_profit),
        "[dim]{:.4f}[/]".format(bh_test_sh),
        "[dim]{:+.2%}[/]".format(bh_test_ap),
    )

    # Absolute drops from training to testing
    sharpe_drop = (test_performance.sharpe_ratio
                   - best_performance.sharpe_ratio)
    profit_drop = test_performance.annual_profit \
                  - best_performance.annual_profit

    perf_table.add_section()
    perf_table.add_row(
        "[dim]Δ (train → test)[/]",
        "[dim]{:+.4f}[/]".format(sharpe_drop),
        "[dim]{:+.2f} pp[/]".format(profit_drop * 100),
        "",
        "",
    )

    console.print(Panel(perf_table, title="[bold]Performance",
                        border_style="bright_blue"))

    # ═══════════════════════════════════════════════════════════════
    # 7. TIMING SUMMARY
    # ═══════════════════════════════════════════════════════════════
    t_total = time.perf_counter() - t0

    time_table = Table(box=box.SIMPLE_HEAVY, show_header=False,
                       border_style="dim")
    time_table.add_column(style="dim", width=28)
    time_table.add_column(style="bright_white", width=14, justify="right")
    time_table.add_column(width=10)

    time_table.add_row("Config loading",  "{:.1f}s".format(t_config),  "")
    time_table.add_row("Price download",  "{:.1f}s".format(t_download),"")
    time_table.add_row("Grid generation", "{:.1f}s".format(t_grid),    "")
    time_table.add_row("Training phase",  "{:.1f}s".format(t_train),
                       "[dim]{:.0%}[/]".format(t_train / t_total))
    time_table.add_row("Testing phase",   "{:.1f}s".format(t_test),
                       "[dim]{:.0%}[/]".format(t_test / t_total))
    time_table.add_row("", "", "")
    time_table.add_row("[bold]Total[/]",  "[bold]{:.1f}s[/]".format(t_total), "")

    console.print(Panel(time_table, title="[bold]Timing",
                        border_style="dim"))

    # ═══════════════════════════════════════════════════════════════
    # 8. VISUALISE (matplotlib — opens figures)
    # ═══════════════════════════════════════════════════════════════
    console.print()
    console.print("  [dim]Opening matplotlib figures …[/]")

    annual_profits = [p.annual_profit for p in performances]
    sharpe_ratios  = [p.sharpe_ratio  for p in performances]

    plot(run, annual_profits, parameter_combos, metric="Annual Profit")
    plot(run, sharpe_ratios,  parameter_combos, metric="Sharpe Ratio")
    show_equity_curve(run, equity_curve)

    console.print("  [dim]Done.[/]")
    console.print()


if __name__ == "__main__":
    main()
