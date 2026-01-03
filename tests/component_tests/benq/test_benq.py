"""Tests for the benq component."""

from collections.abc import Callable

from tests.component_tests.helpers import extract_packed_value


def test_benq_lamp_time_sensor_metadata(generate_main: Callable[[str], str]) -> None:
    """LAMP_TIME must carry the metadata Home Assistant needs for statistics.

    Without a unit and a state class, Home Assistant raises repair issues for the
    entity once it has recorded long term statistics for it.
    """
    main_cpp = generate_main("tests/component_tests/benq/test_benq.yaml")

    # A raw string here would not compile: set_state_class takes an enum.
    assert "set_state_class(sensor::STATE_CLASS_TOTAL_INCREASING)" in main_cpp
    assert "set_accuracy_decimals(0)" in main_cpp

    # Unit, device class, category and icon are packed into the register call.
    # Codegen spells them out in the trailing comment.
    assert extract_packed_value(main_cpp, "lamp_hours") != 0
    assert "uom:h" in main_cpp
    assert "dc:duration" in main_cpp
    assert "category:diagnostic" in main_cpp


def test_benq_defaults_do_not_override_user_values(
    generate_main: Callable[[str], str],
) -> None:
    """Values given in the yaml win over the per-command defaults."""
    main_cpp = generate_main("tests/component_tests/benq/test_benq_override.yaml")

    assert "set_state_class(sensor::STATE_CLASS_MEASUREMENT)" in main_cpp
    assert "set_accuracy_decimals(2)" in main_cpp
    assert "uom:min" in main_cpp
