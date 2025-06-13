from logging import getLogger, basicConfig, DEBUG

logger = getLogger()
basicConfig(level=DEBUG,format="{asctime} - {name} - {levelname} - {message}", style="{")